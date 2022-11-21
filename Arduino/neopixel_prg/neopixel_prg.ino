
#define LOG_OUT 1 // use the log output function
#define FHT_N 256 // set to 256 point fht#include <FHT.h>
#include <FHT.h>

#define NUM_LEDS 50
#define DATA_PIN 11

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
Adafruit_NeoPixel pixels(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);


void setup() {
  Serial.begin(115200);
  Serial.setTimeout(10);

  // neopixel library
  #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
    clock_prescale_set(clock_div_1);
  #endif
  pixels.begin();
  //pixels.setBrightness(64);  // 64 max (0.5 amps) out of 255 (2 amps)
  pixels.setBrightness(25);

  // FHT library (FFT on audio jack input (resistor-pulled-down analog pin 0))
  TIMSK0 = 0; // turn off timer0 for lower jitter
  ADCSRA = 0xe5; // set the adc to free running mode
  ADMUX = 0x40; // use adc0
  DIDR0 = 0x01; // turn off the digital input for adc0

}

typedef struct {
    double r;       // a fraction between 0 and 1
    double g;       // a fraction between 0 and 1
    double b;       // a fraction between 0 and 1
} rgb;

typedef struct {
    double h;       // angle in degrees
    double s;       // a fraction between 0 and 1
    double v;       // a fraction between 0 and 1
} hsv;

static hsv   rgb2hsv(rgb in);
static rgb   hsv2rgb(hsv in);

hsv rgb2hsv(rgb in)
{
    hsv         out;
    double      min, max, delta;

    min = in.r < in.g ? in.r : in.g;
    min = min  < in.b ? min  : in.b;

    max = in.r > in.g ? in.r : in.g;
    max = max  > in.b ? max  : in.b;

    out.v = max;                                // v
    delta = max - min;
    if (delta < 0.00001)
    {
        out.s = 0;
        out.h = 0; // undefined, maybe nan?
        return out;
    }
    if( max > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
        out.s = (delta / max);                  // s
    } else {
        // if max is 0, then r = g = b = 0              
        // s = 0, h is undefined
        out.s = 0.0;
        out.h = NAN;                            // its now undefined
        return out;
    }
    if( in.r >= max )                           // > is bogus, just keeps compilor happy
        out.h = ( in.g - in.b ) / delta;        // between yellow & magenta
    else
    if( in.g >= max )
        out.h = 2.0 + ( in.b - in.r ) / delta;  // between cyan & yellow
    else
        out.h = 4.0 + ( in.r - in.g ) / delta;  // between magenta & cyan

    out.h *= 60.0;                              // degrees

    if( out.h < 0.0 )
        out.h += 360.0;

    return out;
}


rgb hsv2rgb(hsv in)
{
    double      hh, p, q, t, ff;
    long        i;
    rgb         out;

    if(in.s <= 0.0) {       // < is bogus, just shuts up warnings
        out.r = in.v;
        out.g = in.v;
        out.b = in.v;
        return out;
    }
    hh = in.h;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = in.v * (1.0 - in.s);
    q = in.v * (1.0 - (in.s * ff));
    t = in.v * (1.0 - (in.s * (1.0 - ff)));

    switch(i) {
    case 0:
        out.r = in.v;
        out.g = t;
        out.b = p;
        break;
    case 1:
        out.r = q;
        out.g = in.v;
        out.b = p;
        break;
    case 2:
        out.r = p;
        out.g = in.v;
        out.b = t;
        break;

    case 3:
        out.r = p;
        out.g = q;
        out.b = in.v;
        break;
    case 4:
        out.r = t;
        out.g = p;
        out.b = in.v;
        break;
    case 5:
    default:
        out.r = in.v;
        out.g = p;
        out.b = q;
        break;
    }
    return out;     
}

void loop() {

  // FFT
  while(1) { // reduces jitter
    /*
    cli();  // UDRE interrupt slows this way down on arduino1.0*/
    for (int i = 0 ; i < FHT_N ; i++) { // save 256 samples
      //Serial.println("wait start");
      while(!(ADCSRA & 0x10)); // wait for adc to be ready
      //Serial.println("wait end");
      ADCSRA = 0xf5; // restart adc
      byte m = ADCL; // fetch adc data
      byte j = ADCH;
      int k = (j << 8) | m; // form into an int
      k -= 0x0200; // form into a signed int
      k <<= 6; // form into a 16b signed int
      fht_input[i] = k; // put real data into bins
    }
    fht_window(); // window the data for better frequency response
    fht_reorder(); // reorder the data before doing the fht
    fht_run(); // process the data in the fht
    fht_mag_log(); // take the output of the fht
    sei();

    pixels.clear();
    // packet format: fht_log_out={XXX} where X is one of 128 raw bytes
    Serial.print("fht_log_out={");
    
    for (int i = 0; i < 128; i++) {
      // write the raw bytes
      Serial.write(fht_log_out[i]);
      
      if (i < NUM_LEDS) {
        // skip first log cols as they are out of proportion
        int strength = fht_log_out[i+4];
        
        hsv p_hsv;
        //float relative_strength = min(1., strength / 128.);
        float relative_strength = min(1., max(strength - 64, 0) / 64.);
        //float sigmoid_smoothed = 1. / (1. + pow(2.718281828, -relative_strength));
        float sigmoid_smoothed = 1. / (1. + pow(2.718281828, (-8.)*(relative_strength-.5)));
        p_hsv.h = sigmoid_smoothed * 359.;
        p_hsv.s = 1;
        p_hsv.v = min(sigmoid_smoothed / 4., 0.25);
        //p_hsv.v = 0.25 * min(1, strength / 100.);
        rgb p = hsv2rgb(p_hsv);
        
        if (i == 25) {
          //Serial.println(strength);
          //Serial.println((String)p.r+" "+(String)p.g+" "+(String)p.b);
        }
        
        //pixels.setPixelColor(i, pixels.Color(strength > 32 ? strength : 0, 0, 0));
        pixels.setPixelColor(i, pixels.Color(
          relative_strength > 0 ? p.r * 255 : 0, 
          relative_strength > 0 ? p.g * 255 : 0, 
          relative_strength > 0 ? p.b * 255 : 0));
      }
    }
    pixels.show();

    Serial.println("}");
    
  }
}
