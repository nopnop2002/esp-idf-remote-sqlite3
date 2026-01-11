/* The example of ESP-IDF
 *
 * This sample code is in the public domain.
 */
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/ringbuf.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_http_client.h" 
#include "esp_tls.h" 
#include "cJSON.h"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *TAG = "SQLITE";

static int s_retry_num = 0;

EventGroupHandle_t xEventGroup;
/* - Is the Enter key entered? */
const int KEYBOARD_ENTER_BIT = BIT2;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		esp_wifi_connect();
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
			esp_wifi_connect();
			s_retry_num++;
			ESP_LOGI(TAG, "retry to connect to the AP");
		} else {
			xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
		}
		ESP_LOGI(TAG,"connect to the AP fail");
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	}
}

esp_err_t wifi_init_sta(void)
{
	s_wifi_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_sta();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_t instance_got_ip;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
		ESP_EVENT_ANY_ID,
		&event_handler,
		NULL,
		&instance_any_id));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
		IP_EVENT_STA_GOT_IP,
		&event_handler,
		NULL,
		&instance_got_ip));

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = CONFIG_ESP_WIFI_SSID,
			.password = CONFIG_ESP_WIFI_PASSWORD,
			.scan_method = WIFI_ALL_CHANNEL_SCAN,
			.failure_retry_cnt = CONFIG_ESP_MAXIMUM_RETRY,
			/* Setting a password implies station will connect to all security modes including WEP/WPA.
			 * However these modes are deprecated and not advisable to be used. Incase your Access point
			 * doesn't support WPA2, these mode can be enabled by commenting below line */
			.threshold.authmode = WIFI_AUTH_WPA2_PSK,

			.pmf_cfg = {
				.capable = true,
				.required = false
			},
		},
	};
	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	/* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
	 * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
	esp_err_t ret_value = ESP_OK;
	EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
		WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
		pdFALSE,
		pdFALSE,
		portMAX_DELAY);

	/* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
	 * happened. */
	if (bits & WIFI_CONNECTED_BIT) {
		ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
	} else if (bits & WIFI_FAIL_BIT) {
		ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
		ret_value = ESP_FAIL;
	} else {
		ESP_LOGE(TAG, "UNEXPECTED EVENT");
		ret_value = ESP_FAIL;
	}

	/* The event will not be processed after unregister */
	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
	vEventGroupDelete(s_wifi_event_group);
	return ret_value;
}

void JSON_Record(const cJSON * const array) {
	int id = cJSON_GetObjectItem(array,"id")->valueint;
	char *name = cJSON_GetObjectItem(array,"name")->valuestring;
	int gender = cJSON_GetObjectItem(array,"gender")->valueint;
	ESP_LOGI(TAG, "%d\t%s\t%d", id, name, gender);
}

char *JSON_Types(int type) {
	if (type == cJSON_Invalid) return ("cJSON_Invalid");
	if (type == cJSON_False) return ("cJSON_False");
	if (type == cJSON_True) return ("cJSON_True");
	if (type == cJSON_NULL) return ("cJSON_NULL");
	if (type == cJSON_Number) return ("cJSON_Number");
	if (type == cJSON_String) return ("cJSON_String");
	if (type == cJSON_Array) return ("cJSON_Array");
	if (type == cJSON_Object) return ("cJSON_Object");
	if (type == cJSON_Raw) return ("cJSON_Raw");
	return NULL;
}

void JSON_Print(const cJSON * const root) {
	ESP_LOGI(TAG, "-----------------------------------------");
	ESP_LOGD(TAG, "root->type=%s", JSON_Types(root->type));
	if (cJSON_IsArray(root)) {
		ESP_LOGD(TAG, "root->type is Array");
		int root_array_size = cJSON_GetArraySize(root); 
		for (int i=0;i<root_array_size;i++) {
			cJSON *record = cJSON_GetArrayItem(root,i);
			JSON_Record(record);
		}
	} else {
		ESP_LOGD(TAG, "root->type is Object");
		JSON_Record(root);
	}
	ESP_LOGI(TAG, "-----------------------------------------");
}

void JSON_Analyze(const cJSON * const root) {
	//ESP_LOGI(TAG, "root->type=%s", JSON_Types(root->type));
	cJSON *current_element = NULL;
	//ESP_LOGI(TAG, "root->child=%p", root->child);
	//ESP_LOGI(TAG, "root->next =%p", root->next);
	cJSON_ArrayForEach(current_element, root) {
		//ESP_LOGI(TAG, "type=%s", JSON_Types(current_element->type));
		//ESP_LOGI(TAG, "current_element->string=%p", current_element->string);
		if (current_element->string) {
			const char* string = current_element->string;
			ESP_LOGI(TAG, "[%s]", string);
		}
		if (cJSON_IsInvalid(current_element)) {
			ESP_LOGI(TAG, "Invalid");
		} else if (cJSON_IsFalse(current_element)) {
			ESP_LOGI(TAG, "False");
		} else if (cJSON_IsTrue(current_element)) {
			ESP_LOGI(TAG, "True");
		} else if (cJSON_IsNull(current_element)) {
			ESP_LOGI(TAG, "Null");
		} else if (cJSON_IsNumber(current_element)) {
			int valueint = current_element->valueint;
			double valuedouble = current_element->valuedouble;
			ESP_LOGI(TAG, "int=%d double=%f", valueint, valuedouble);
		} else if (cJSON_IsString(current_element)) {
			const char* valuestring = current_element->valuestring;
			ESP_LOGI(TAG, "%s", valuestring);
		} else if (cJSON_IsArray(current_element)) {
			ESP_LOGD(TAG, "Array");
			JSON_Analyze(current_element);
		} else if (cJSON_IsObject(current_element)) {
			ESP_LOGD(TAG, "Object");
			JSON_Analyze(current_element);
		} else if (cJSON_IsRaw(current_element)) {
			ESP_LOGI(TAG, "Raw(Not support)");
		}
	}
}

#define MAX_HTTP_OUTPUT_BUFFER 2048

/*
 * http_native_request() demonstrates use of low level APIs to connect to a server,
 * make a http request and read response. Event handler is not used in this case.
 * Note: This approach should only be used in case use of low level APIs is required.
 * By default, PHP's built-in web server does not automatically send a Content-Length header for dynamic content.
 */
esp_err_t sqlite3_client_get(char * path)
{
	ESP_LOGI(TAG, "sqlite3_client_get path=%s",path);
	char output_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
	char url[64];
	sprintf(url, "http://%s:%d/", CONFIG_ESP_WEB_SERVER, CONFIG_ESP_WEB_SERVER_PORT);
	if (strlen(path) > 0) {
		int url_length = strlen(url);
		if (url[url_length-1] != '/') strcat(url,"/");
		strcat(url, path);
	}
	ESP_LOGI(TAG, "url=[%s]",url);
	
	esp_http_client_config_t config = {
		.url = url,
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// GET Request
	esp_err_t ret = ESP_FAIL;
	esp_http_client_set_method(client, HTTP_METHOD_GET);
	esp_err_t err = esp_http_client_open(client, 0);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
	} else {
		int64_t content_length = esp_http_client_fetch_headers(client);
		ESP_LOGI(TAG, "HTTP GET content_length = %"PRId64, content_length);
		if (content_length < 0) {
			ESP_LOGE(TAG, "HTTP client fetch headers failed");
		} else {
			int data_read = esp_http_client_read_response(client, output_buffer, MAX_HTTP_OUTPUT_BUFFER);
			if (data_read >= 0) {
				ESP_LOGI(TAG, "HTTP GET Status = %d, data_read = %d",
					esp_http_client_get_status_code(client), data_read);
				ESP_LOG_BUFFER_HEXDUMP(TAG, output_buffer, data_read, ESP_LOG_DEBUG);
				output_buffer[data_read] = 0;
				ESP_LOGD(TAG, "\n%s", output_buffer);
				cJSON *root = cJSON_Parse(output_buffer);
				JSON_Print(root);
				ret = ESP_OK;
			} else {
				ESP_LOGE(TAG, "HTTP client read response failed");
			}
		}
	}
	esp_http_client_close(client);
	esp_http_client_cleanup(client);
	return ret;
}

esp_err_t sqlite3_client_post(char * path, char * name, int gender)
{
	ESP_LOGI(TAG, "sqlite3_client_post path=%s",path);
	char output_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
	char url[64];
	sprintf(url, "http://%s:%d/", CONFIG_ESP_WEB_SERVER, CONFIG_ESP_WEB_SERVER_PORT);
	if (strlen(path) > 0) {
		int url_length = strlen(url);
		if (url[url_length-1] != '/') strcat(url,"/");
		strcat(url, path);
	}
	ESP_LOGI(TAG, "url=[%s]",url);
	
	esp_http_client_config_t config = {
		.url = url,
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// POST Request
	esp_err_t ret = ESP_FAIL;
	char post_data[64];
	//sprintf(post_data, "name=%s&gender=%d", name, gender);
	sprintf(post_data, "{\"name\":\"%s\", \"gender\":%d}", name, gender);
	ESP_LOGI(TAG, "post_data=[%s]", post_data);
	esp_http_client_set_method(client, HTTP_METHOD_POST);
	esp_http_client_set_header(client, "Content-Type", "application/json");
	esp_err_t err = esp_http_client_open(client, strlen(post_data));
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
	} else {
		int wlen = esp_http_client_write(client, post_data, strlen(post_data));
		if (wlen < 0) {
			ESP_LOGE(TAG, "HTTP client write failed");
		}
		int64_t content_length = esp_http_client_fetch_headers(client);
		if (content_length < 0) {
			ESP_LOGE(TAG, "HTTP client fetch headers failed");
		} else {
			int data_read = esp_http_client_read_response(client, output_buffer, MAX_HTTP_OUTPUT_BUFFER);
			if (data_read >= 0) {
				ESP_LOGI(TAG, "HTTP POST Status = %d, data_read=%d",
					esp_http_client_get_status_code(client), data_read);
				ESP_LOG_BUFFER_HEXDUMP(TAG, output_buffer, data_read, ESP_LOG_DEBUG);
				output_buffer[data_read] = 0;
				ESP_LOGD(TAG, "\n%s", output_buffer);
				cJSON *root = cJSON_Parse(output_buffer);
				char *response_string = cJSON_Print(root);
				ESP_LOGI(TAG, "response_string\n%s",response_string);
				cJSON_Delete(root);
				cJSON_free(response_string);
				ret = ESP_OK;
			} else {
				ESP_LOGE(TAG, "HTTP client read response failed");
			}
		}
	}
	esp_http_client_close(client);
	esp_http_client_cleanup(client);
	return ret;
}

esp_err_t sqlite3_client_put(char * path, char * name, int gender)
{
	ESP_LOGI(TAG, "sqlite3_client_put path=%s",path);
	char output_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
	char url[64];
	sprintf(url, "http://%s:%d/", CONFIG_ESP_WEB_SERVER, CONFIG_ESP_WEB_SERVER_PORT);
	if (strlen(path) > 0) {
		int url_length = strlen(url);
		if (url[url_length-1] != '/') strcat(url,"/");
		strcat(url, path);
	}
	ESP_LOGI(TAG, "url=[%s]",url);

	esp_http_client_config_t config = {
		.url = url,
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// PUT Request
	esp_err_t ret = ESP_FAIL;
	char post_data[64];
	//sprintf(post_data, "name=%s&gender=%d", name, gender);
	sprintf(post_data, "{\"name\":\"%s\", \"gender\":%d}", name, gender);
	ESP_LOGI(TAG, "post_data=[%s]", post_data);
	esp_http_client_set_method(client, HTTP_METHOD_PUT);
	esp_http_client_set_header(client, "Content-Type", "application/json");
	esp_err_t err = esp_http_client_open(client, strlen(post_data));
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
	} else {
		int wlen = esp_http_client_write(client, post_data, strlen(post_data));
		if (wlen < 0) {
			ESP_LOGE(TAG, "HTTP client write failed");
		}
		int64_t content_length = esp_http_client_fetch_headers(client);
		if (content_length < 0) {
			ESP_LOGE(TAG, "HTTP client fetch headers failed");
		} else {
			int data_read = esp_http_client_read_response(client, output_buffer, MAX_HTTP_OUTPUT_BUFFER);
			if (data_read >= 0) {
				ESP_LOGI(TAG, "HTTP PUT Status = %d, data_read=%d",
					esp_http_client_get_status_code(client), data_read);
				ESP_LOG_BUFFER_HEXDUMP(TAG, output_buffer, data_read, ESP_LOG_DEBUG);
				output_buffer[data_read] = 0;
				ESP_LOGD(TAG, "\n%s", output_buffer);
				cJSON *root = cJSON_Parse(output_buffer);
				char *response_string = cJSON_Print(root);
				ESP_LOGI(TAG, "response_string\n%s",response_string);
				cJSON_Delete(root);
				cJSON_free(response_string);
				ret = ESP_OK;
			} else {
				ESP_LOGE(TAG, "HTTP client read response failed");
			}
		}
	}
	esp_http_client_close(client);
	esp_http_client_cleanup(client);
	return ret;
}

esp_err_t sqlite3_client_delete(char * path)
{
	ESP_LOGI(TAG, "sqlite3_client_delete path=%s",path);
	char output_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
	char url[64];
	sprintf(url, "http://%s:%d/", CONFIG_ESP_WEB_SERVER, CONFIG_ESP_WEB_SERVER_PORT);
	if (strlen(path) > 0) {
		int url_length = strlen(url);
		if (url[url_length-1] != '/') strcat(url,"/");
		strcat(url, path);
	}
	ESP_LOGI(TAG, "url=[%s]",url);

	esp_http_client_config_t config = {
		.url = url,
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// DELETE Request
	esp_err_t ret = ESP_FAIL;
	esp_http_client_set_method(client, HTTP_METHOD_DELETE);
	esp_err_t err = esp_http_client_open(client, 0);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
	} else {
		int wlen = esp_http_client_write(client, "", 0);
		if (wlen < 0) {
			ESP_LOGE(TAG, "HTTP client write failed");
		}
		int64_t content_length = esp_http_client_fetch_headers(client);
		if (content_length < 0) {
			ESP_LOGE(TAG, "HTTP client fetch headers failed");
		} else {
			int data_read = esp_http_client_read_response(client, output_buffer, MAX_HTTP_OUTPUT_BUFFER);
			if (data_read >= 0) {
				ESP_LOGI(TAG, "HTTP DELETE Status = %d, data_read=%d",
					esp_http_client_get_status_code(client), data_read);
				ESP_LOG_BUFFER_HEXDUMP(TAG, output_buffer, data_read, ESP_LOG_DEBUG);
				output_buffer[data_read] = 0;
				ESP_LOGD(TAG, "\n%s", output_buffer);
				cJSON *root = cJSON_Parse(output_buffer);
				char *response_string = cJSON_Print(root);
				ESP_LOGI(TAG, "response_string\n%s",response_string);
				cJSON_Delete(root);
				cJSON_free(response_string);
				ret = ESP_OK;
			} else {
				ESP_LOGE(TAG, "HTTP client read response failed");
			}
		}
	}
	esp_http_client_close(client);
	esp_http_client_cleanup(client);
	return ret;
}

int sqlite3_client_get_maxid(char * path)
{
	ESP_LOGI(TAG, "sqlite3_client_get_maxid path=%s",path);
	char output_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
	char _path[64];
	sprintf(_path, "/%s?limit=1&by=id&order=desc", path);
	ESP_LOGI(TAG, "_path=[%s]", _path);
	esp_http_client_config_t config = {
		.host = CONFIG_ESP_WEB_SERVER,
		.port = CONFIG_ESP_WEB_SERVER_PORT,
		//.path = "/customers/?limit=1&by=id&order=desc",
		.path = _path,
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// GET Request
	int newid = -1;
	esp_http_client_set_method(client, HTTP_METHOD_GET);
	esp_err_t err = esp_http_client_open(client, 0);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
	} else {
		int64_t content_length = esp_http_client_fetch_headers(client);
		ESP_LOGI(TAG, "HTTP GET content_length = %"PRId64, content_length);
		if (content_length < 0) {
			ESP_LOGE(TAG, "HTTP client fetch headers failed");
		} else {
			int data_read = esp_http_client_read_response(client, output_buffer, MAX_HTTP_OUTPUT_BUFFER);
			if (data_read >= 0) {
				ESP_LOGI(TAG, "HTTP GET Status = %d, data_read = %d",
					esp_http_client_get_status_code(client), data_read);
				ESP_LOG_BUFFER_HEXDUMP(TAG, output_buffer, data_read, ESP_LOG_DEBUG);
				output_buffer[data_read] = 0;
				ESP_LOGD(TAG, "\n%s", output_buffer);
				cJSON *root = cJSON_Parse(output_buffer);
				int root_array_size = cJSON_GetArraySize(root); 
				ESP_LOGD(TAG, "root_array_size=%d", root_array_size);
				for (int i=0;i<root_array_size;i++) {
					cJSON *array = cJSON_GetArrayItem(root,i);
					newid = cJSON_GetObjectItem(array,"id")->valueint;
					ESP_LOGD(TAG, "newid=%d",newid);
				}
				cJSON_Delete(root);
			} else {
				ESP_LOGE(TAG, "HTTP client read response failed");
			}
		}
	}
	esp_http_client_close(client);
	esp_http_client_cleanup(client);
	return newid;
}

void http_task(void *pvParameters)
{
	// Read all data
	ESP_LOGI(TAG, "");
	ESP_LOGW(TAG, "Enter key to Read all data");
	xEventGroupClearBits(xEventGroup, KEYBOARD_ENTER_BIT);
	xEventGroupWaitBits(xEventGroup, KEYBOARD_ENTER_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	sqlite3_client_get("customers/"); 

	// Read by ID
	ESP_LOGI(TAG, "");
	ESP_LOGW(TAG, "Enter key to Read by ID");
	xEventGroupClearBits(xEventGroup, KEYBOARD_ENTER_BIT);
	xEventGroupWaitBits(xEventGroup, KEYBOARD_ENTER_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	sqlite3_client_get("customers/3"); 

	// Read by gender
	ESP_LOGI(TAG, "");
	ESP_LOGW(TAG, "Enter key to Read by gender");
	xEventGroupClearBits(xEventGroup, KEYBOARD_ENTER_BIT);
	xEventGroupWaitBits(xEventGroup, KEYBOARD_ENTER_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	sqlite3_client_get("customers/gender/2"); 

	// Create
	ESP_LOGI(TAG, "");
	ESP_LOGW(TAG, "Enter key to Create new record");
	xEventGroupClearBits(xEventGroup, KEYBOARD_ENTER_BIT);
	xEventGroupWaitBits(xEventGroup, KEYBOARD_ENTER_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	int newid = 0;
	char path[64];
	if (sqlite3_client_post("customers/", "Tom", 1) == ESP_OK) {
		newid = sqlite3_client_get_maxid("customers/"); 
		ESP_LOGI(TAG, "newid=%d", newid);
		sprintf(path, "customers/%d", newid);
		sqlite3_client_get(path); 
	}

	// Update
	ESP_LOGI(TAG, "");
	ESP_LOGW(TAG, "Enter key to Update new record");
	xEventGroupClearBits(xEventGroup, KEYBOARD_ENTER_BIT);
	xEventGroupWaitBits(xEventGroup, KEYBOARD_ENTER_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	sprintf(path, "customers/%d", newid);
	if(sqlite3_client_get(path) == ESP_OK) { 
		sqlite3_client_put(path, "Petty", 2); 
		sqlite3_client_get(path); 
	}

	// Delete
	ESP_LOGI(TAG, "");
	ESP_LOGW(TAG, "Enter key to Delete new record");
	xEventGroupClearBits(xEventGroup, KEYBOARD_ENTER_BIT);
	xEventGroupWaitBits(xEventGroup, KEYBOARD_ENTER_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	if(sqlite3_client_get(path) == ESP_OK) { 
		sqlite3_client_delete(path); 
		sqlite3_client_get("customers"); 
	}
	ESP_LOGI(TAG, "All Finish!!");
	vTaskDelete(NULL);
}

void keyin_task(void *pvParameters)
{
	//ESP_LOGI(pcTaskGetName(NULL), "Start");
	uint16_t c;
	while (1) {
		c = fgetc(stdin);
		if (c == 0xffff) {
			vTaskDelay(10);
			continue;
		}
		//ESP_LOGI(pcTaskGetName(NULL), "c=%x", c);
		if (c == 0x0a) {
			ESP_LOGD(pcTaskGetName(NULL), "Push Enter");
			xEventGroupSetBits(xEventGroup, KEYBOARD_ENTER_BIT);
		}
	}
	vTaskDelete(NULL);
}

void app_main()
{
	// Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	// Initialize WiFi
	ESP_ERROR_CHECK(wifi_init_sta());

	// Create EventGroup
	xEventGroup = xEventGroupCreate();

	// Check everything was created
	configASSERT( xEventGroup );

	// Start keyboard task
	xTaskCreate(keyin_task, "KEYIN", 1024*2, NULL, 2, NULL);

	// Start http task
	xTaskCreate(http_task, "HTTP", 1024*8, NULL, 2, NULL);
}
