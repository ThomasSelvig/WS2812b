-- init the ws2812 module
ws2812.init(ws2812.MODE_SINGLE)
-- create a buffer, 50 LEDs with 3 color bytes
strip_buffer = ws2812.newBuffer(50, 3)
-- init the effects module, set color to red and start blinking
ws2812_effects.init(strip_buffer)
ws2812_effects.set_speed(200)
ws2812_effects.set_brightness(32)
-- ws2812_effects.set_color(0,255,0)
ws2812_effects.set_mode("rainbow_cycle")
ws2812_effects.start()