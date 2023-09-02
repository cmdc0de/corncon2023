#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include <nvs_memory.h>

#include "system.h"
#include "i2c.hpp"
#include <esp_log.h>

#include "app.h"

extern "C" {
	void app_main();
}

#define ESP_INTR_FLAG_DEFAULT 0


void app_main() {
	static const char *LOGTAG="APP_MAIN";
	esp_err_t ret;
	
	// initialize NVS
	ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase()); // TODO: do we actually want this?
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK( ret );

//	gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

   gpio_install_isr_service(0);
	libesp::ErrorType et;
	et = MyApp::get().init();

	if(!et.ok()) {
		ESP_LOGE(LOGTAG,"init error: %s", et.toString());
	}


	libesp::System::get().logSystemInfo();

	do {
		et = MyApp::get().run();
		//vTaskDelay(1 / portTICK_RATE_MS);
	} while (et.ok());
	vTaskDelay(3000 / portTICK_RATE_MS);
}

