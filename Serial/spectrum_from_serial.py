import serial
import re
import pygame
import colorsys


pygame.init()
fps = 30
clock = pygame.time.Clock()
# w, h = int(1920*2.8), 300
w, h = 1000, 300
screen = pygame.display.set_mode((w, h))
pygame.display.set_caption("Audio Spectrum Analyzer")

exit_game = False


def render_spectrum(fht_log_out):
	for i, fht in enumerate(fht_log_out):
		
		candle_width = 25
		width = candle_width if w / len(fht_log_out) > candle_width else w / len(fht_log_out)
		total_pad = w / len(fht_log_out) - width
		
		# relative_strength = min((1, fht / 128))
		relative_strength = min((1, max(fht - 64, 0) / 64))
		# sigmoid_smoothed = 1 / (1 + pow(math.e, (-8)*(relative_strength - .5)))
		sigmoid_smoothed = 1 / (1 + pow(2.718281828, (-8)*(relative_strength - .5)))
		hue = sigmoid_smoothed

		brightness = min(sigmoid_smoothed, 1)
		pygame.draw.rect(
			screen,
			# [255, 0, 0],
			# [i * 255 for i in colorsys.hsv_to_rgb(fht / 255, 1, 1)],
			# [i * 255 for i in colorsys.hsv_to_rgb(fht / 255, 1, 1)] if \
			[i * 255 * brightness for i in colorsys.hsv_to_rgb(hue, 1, 1)] if \
				i < 50 else \
				[255, 255, 255],
			[
				i * (total_pad+width) + total_pad / 2, # i * (w / len(fht_log_out)),  # left
				h - fht,  # top
				width,  # w / len(fht_log_out),  # width
				fht  # height
			]
		)

with serial.Serial("COM4", 115200) as arduino:
	# Creating a game loop
	while not exit_game:

		# event handling
		for event in pygame.event.get():
			if event.type == pygame.QUIT:
				exit_game = True
			if event.type == pygame.KEYDOWN:
				if event.key == pygame.K_ESCAPE:
					pygame.quit()
					quit()

		# parse packet from serial
		raw_packet = arduino.readline()
		packet = str(raw_packet, "utf8", errors="replace").strip()
		if (fht_log_out_raw := re.search(r"(?<=fht_log_out\=\{)(.*?)(?=\})", packet)):
			# fht_log_out packet found, parse the 128 raw bytes in between the "{}"
			span = fht_log_out_raw.span()
			fht_log_out = [i for i in raw_packet[span[0]:span[1]]]

			screen.fill((0, 0, 0))
			render_spectrum(fht_log_out[4:54])
			pygame.draw.line(screen, (255, 0, 0), (0, h-64), (w, h-64))  # red y=50 (threshold)
			pygame.draw.line(screen, (0, 255, 0), (0, h-128), (w, h-128))  # green y=255
			pygame.display.flip()
			# clock.tick(fps)


	pygame.quit()
	quit()
