HOMEKIT_ACCESSORY_CATEGORY = 9 # Thermostat
HOMEKIT_SETUP_CODE = 343-10-202 # Make sure to also update this in the `idf.py menuconfig`
HOMEKIT_SETUP_ID = XY38

qrcode/homekit:
	./components/esp32-homekit/tools/gen_qrcode \
		$(HOMEKIT_ACCESSORY_CATEGORY) $(HOMEKIT_SETUP_CODE) $(HOMEKIT_SETUP_ID) assets/homekit-qrcode.png