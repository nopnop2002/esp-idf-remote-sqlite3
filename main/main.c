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


/* The examples use WiFi configuration that you can set via project configuration menu

	 If you'd rather not, just change the below entries to strings with
	 the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID			CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS			CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY		CONFIG_ESP_MAXIMUM_RETRY

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about one event */
/* - Are we connected to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;


static const char *TAG = "SQLITE";

static int s_retry_num = 0;

EventGroupHandle_t xEventGroup;
/* - Is the Enter key entered? */
const int KEYBOARD_ENTER_BIT = BIT2;


esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
	static char *output_buffer;  // Buffer to store response of http request from event handler
	static int output_len;		 // Stores number of bytes read
	switch(evt->event_id) {
		case HTTP_EVENT_ERROR:
			ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
			break;
		case HTTP_EVENT_ON_CONNECTED:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
			break;
		case HTTP_EVENT_HEADER_SENT:
			ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
			break;
		case HTTP_EVENT_ON_HEADER:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
			break;
		case HTTP_EVENT_ON_DATA:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
			ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, output_len=%d", output_len);
			ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, content_length=%d", esp_http_client_get_content_length(evt->client));
#if 0
			/*
			 *	Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
			 *	However, event handler can also be used in case chunked encoding is used.
			 */
			if (!esp_http_client_is_chunked_response(evt->client)) {
#endif
				// If user_data buffer is configured, copy the response into the buffer
				if (evt->user_data) {
					memcpy(evt->user_data + output_len, evt->data, evt->data_len);
				} else {
					if (output_buffer == NULL) {
						output_buffer = (char *) malloc(esp_http_client_get_content_length(evt->client));
						output_len = 0;
						if (output_buffer == NULL) {
							ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
							return ESP_FAIL;
						}
					}
					memcpy(output_buffer + output_len, evt->data, evt->data_len);
				}
				output_len += evt->data_len;
#if 0
			}
#endif
			break;
		case HTTP_EVENT_ON_FINISH:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
			if (output_buffer != NULL) {
				// Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
				// ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
				free(output_buffer);
				output_buffer = NULL;
			}
			output_len = 0;
			break;
		case HTTP_EVENT_DISCONNECTED:
			ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
#if 0
			int mbedtls_err = 0;
			esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
			ESP_LOGI(TAG, "esp_tls_get_and_clear_last_error=%d", err);
			if (err != 0) {
				if (output_buffer != NULL) {
					free(output_buffer);
					output_buffer = NULL;
				}
				output_len = 0;
				ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
				ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
			}
#endif
			if (output_buffer != NULL) {
				free(output_buffer);
				output_buffer = NULL;
			}
			break;
	}
	return ESP_OK;
}

static void event_handler(void* arg, esp_event_base_t event_base,
								int32_t event_id, void* event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		esp_wifi_connect();
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
			esp_wifi_connect();
			xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
			s_retry_num++;
			ESP_LOGI(TAG, "retry to connect to the AP");
		}
		ESP_LOGE(TAG,"connect to the AP fail");
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		//ESP_LOGI(TAG, "got ip:%s", ip4addr_ntoa(&event->ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
	}
}

void wifi_init_sta()
{
	wifi_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_sta();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = EXAMPLE_ESP_WIFI_SSID,
			.password = EXAMPLE_ESP_WIFI_PASS
		},
	};
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
	ESP_ERROR_CHECK(esp_wifi_start() );

	xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	ESP_LOGI(TAG, "wifi_init_sta finished.");
	ESP_LOGI(TAG, "connect to ap SSID:%s", EXAMPLE_ESP_WIFI_SSID);
}

void JSON_Record(const cJSON * const array) {
		char *id = cJSON_GetObjectItem(array,"id")->valuestring;
		char *name = cJSON_GetObjectItem(array,"name")->valuestring;
		char *gender = cJSON_GetObjectItem(array,"gender")->valuestring;
		ESP_LOGI(TAG, "%s\t%s\t%s", id, name, gender);
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

esp_err_t sqlite3_client_get(char * path)
{
	ESP_LOGI(TAG, "sqlite3_client_get path=%s",path);
	char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
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
		.event_handler = _http_event_handler,
		.user_data = local_response_buffer, // Pass address of local buffer to get response
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// GET
	esp_err_t ret;
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
				esp_http_client_get_status_code(client),
				esp_http_client_get_content_length(client));
		ESP_LOGD(TAG, "local_response_buffer=%d", strlen(local_response_buffer));
		ESP_LOGD(TAG, "%s", local_response_buffer);

		ESP_LOGI(TAG, "Deserialize.....");
		cJSON *root = cJSON_Parse(local_response_buffer);
		JSON_Print(root);
		cJSON_Delete(root);
		ret = ESP_OK;
	} else {
		ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
		ret = ESP_FAIL;
	}
	esp_http_client_cleanup(client);
	return ret;
}

esp_err_t sqlite3_client_post(char * path, char * name, int gender)
{
	ESP_LOGI(TAG, "sqlite3_client_post path=%s",path);
	char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
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
		.event_handler = _http_event_handler,
		.user_data = local_response_buffer, // Pass address of local buffer to get response
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// POST
	char post_data[64];
	sprintf(post_data, "name=%s&gender=%d", name, gender);
	ESP_LOGI(TAG, "post_data=[%s]", post_data);
	esp_http_client_set_url(client, url);
	esp_http_client_set_method(client, HTTP_METHOD_POST);
	esp_http_client_set_post_field(client, post_data, strlen(post_data));
	esp_err_t ret;
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		ESP_LOGD(TAG, "HTTP GET Status = %d, content_length = %d",
				esp_http_client_get_status_code(client),
				esp_http_client_get_content_length(client));
		ESP_LOGD(TAG, "local_response_buffer=%d", strlen(local_response_buffer));
		ESP_LOGD(TAG, "%s", local_response_buffer);
		ret = ESP_OK;
		
	} else {
		ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
	}
	esp_http_client_cleanup(client);
	return ret;
}

esp_err_t sqlite3_client_put(char * path, char * name, int gender)
{
	ESP_LOGI(TAG, "sqlite3_client_put path=%s",path);
	char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
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
		.event_handler = _http_event_handler,
		.user_data = local_response_buffer, // Pass address of local buffer to get response
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// PUT
	char post_data[64];
	sprintf(post_data, "name=%s&gender=%d", name, gender);
	ESP_LOGI(TAG, "post_data=[%s]", post_data);
	esp_http_client_set_url(client, url);
	esp_http_client_set_method(client, HTTP_METHOD_PUT);
	esp_http_client_set_post_field(client, post_data, strlen(post_data));
	esp_err_t ret;
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		ESP_LOGD(TAG, "HTTP GET Status = %d, content_length = %d",
				esp_http_client_get_status_code(client),
				esp_http_client_get_content_length(client));
		ESP_LOGD(TAG, "local_response_buffer=%d", strlen(local_response_buffer));
		ESP_LOGD(TAG, "%s", local_response_buffer);
		ret = ESP_OK;
	} else {
		ESP_LOGE(TAG, "HTTP PUT request failed: %s", esp_err_to_name(err));
		ret = ESP_FAIL;
	}
	esp_http_client_cleanup(client);
	return ret;
}

esp_err_t sqlite3_client_delete(char * path)
{
	ESP_LOGI(TAG, "sqlite3_client_delete path=%s",path);
	char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
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
		.event_handler = _http_event_handler,
		.user_data = local_response_buffer, // Pass address of local buffer to get response
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// DELETE
	esp_http_client_set_url(client, url);
	esp_http_client_set_method(client, HTTP_METHOD_DELETE);
	esp_err_t ret;
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		ESP_LOGD(TAG, "HTTP GET Status = %d, content_length = %d",
				esp_http_client_get_status_code(client),
				esp_http_client_get_content_length(client));
		ESP_LOGD(TAG, "local_response_buffer=%d", strlen(local_response_buffer));
		ESP_LOGD(TAG, "%s", local_response_buffer);
		ret = ESP_OK;
	} else {
		ESP_LOGE(TAG, "HTTP DELETE request failed: %s", esp_err_to_name(err));
		ret = ESP_FAIL;
	}
	esp_http_client_cleanup(client);
	return ret;
}

int sqlite3_client_maxid(char * path)
{
	ESP_LOGI(TAG, "sqlite3_client_maxid path=%s",path);
	char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
#if 0
	char url[64];
	sprintf(url, "http://%s:%d/", CONFIG_ESP_WEB_SERVER, CONFIG_ESP_WEB_SERVER_PORT);
	if (strlen(path) > 0) {
		int url_length = strlen(url);
		if (url[url_length-1] != '/') strcat(url,"/");
		strcat(url, path);
	}
	// SELECT * FROM "customers" ORDER BY "id" desc LIMIT 1;
	strcat(url, "?limit=1&by=id&order=desc");
	ESP_LOGI(TAG, "url=[%s]",url);

	esp_http_client_config_t config = {
		.url = url,
		.event_handler = _http_event_handler,
		.user_data = local_response_buffer,	// Pass address of local buffer to get response
	};

#if 0
	esp_http_client_config_t config = {
		.url = "http://lenovo-s500.local:8080/customers/?limit=1&by=id&order=desc",
		.event_handler = _http_event_handler,
		.user_data = local_response_buffer,	// Pass address of local buffer to get response
	};
#endif
#endif

#if 1
	char _path[64];
	sprintf(_path, "/%slimit=1&by=id&order=desc", path);
	ESP_LOGI(TAG, "_path=[%s]", _path);
	esp_http_client_config_t config = {
		.host = CONFIG_ESP_WEB_SERVER,
		.port = CONFIG_ESP_WEB_SERVER_PORT,
		//.path = "/customers/?limit=1&by=id&order=desc",
		.path = _path,
		.event_handler = _http_event_handler,
		.user_data = local_response_buffer,	// Pass address of local buffer to get response
	};
#endif
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// GET
	int rcode = -1;
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		ESP_LOGD(TAG, "HTTP GET Status = %d, content_length = %d",
				esp_http_client_get_status_code(client),
				esp_http_client_get_content_length(client));
		ESP_LOGD(TAG, "local_response_buffer=%d", strlen(local_response_buffer));
		ESP_LOGD(TAG, "%s", local_response_buffer);

		ESP_LOGD(TAG, "Deserialize.....");
		cJSON *root = cJSON_Parse(local_response_buffer);
		int root_array_size = cJSON_GetArraySize(root); 
		ESP_LOGD(TAG, "root_array_size=%d", root_array_size);
		for (int i=0;i<root_array_size;i++) {
			cJSON *array = cJSON_GetArrayItem(root,i);
			char *id = cJSON_GetObjectItem(array,"id")->valuestring;
			ESP_LOGD(TAG, "id=%s",id);
			rcode = atoi(id);
		}
		cJSON_Delete(root);
	} else {
		ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
	}
	esp_http_client_cleanup(client);
	return rcode;
}

void http_task(void *pvParameters)
{
	// Read all data
	ESP_LOGI(TAG, "");
	ESP_LOGI(TAG, "Enter key to Read all data");
	xEventGroupClearBits(xEventGroup, KEYBOARD_ENTER_BIT);
	xEventGroupWaitBits(xEventGroup, KEYBOARD_ENTER_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	sqlite3_client_get("customers/"); 

	// Read by ID
	ESP_LOGI(TAG, "");
	ESP_LOGI(TAG, "Enter key to Read by ID");
	xEventGroupClearBits(xEventGroup, KEYBOARD_ENTER_BIT);
	xEventGroupWaitBits(xEventGroup, KEYBOARD_ENTER_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	sqlite3_client_get("customers/3"); 

	// Read by gender
	ESP_LOGI(TAG, "");
	ESP_LOGI(TAG, "Enter key to Read by gender");
	xEventGroupClearBits(xEventGroup, KEYBOARD_ENTER_BIT);
	xEventGroupWaitBits(xEventGroup, KEYBOARD_ENTER_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	sqlite3_client_get("customers/gender/2"); 

	//Create
	ESP_LOGI(TAG, "");
	ESP_LOGI(TAG, "Enter key to Create new record");
	xEventGroupClearBits(xEventGroup, KEYBOARD_ENTER_BIT);
	xEventGroupWaitBits(xEventGroup, KEYBOARD_ENTER_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	int newid = 0;
	char path[64];
	if (sqlite3_client_post("customers/", "Tom", 1) == ESP_OK) {
		newid = sqlite3_client_maxid("customers/"); 
		ESP_LOGI(TAG, "newid=%d", newid);
		sprintf(path, "customers/%d", newid);
		sqlite3_client_get(path); 
	}

	//Update
	ESP_LOGI(TAG, "");
	ESP_LOGI(TAG, "Enter key to Update new record");
	xEventGroupClearBits(xEventGroup, KEYBOARD_ENTER_BIT);
	xEventGroupWaitBits(xEventGroup, KEYBOARD_ENTER_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	sprintf(path, "customers/%d", newid);
	if(sqlite3_client_get(path) == ESP_OK) { 
		sqlite3_client_put(path, "Petty", 2); 
		sqlite3_client_get(path); 
	}

	//Delete
	ESP_LOGI(TAG, "");
	ESP_LOGI(TAG, "Enter key to Delete new record");
	xEventGroupClearBits(xEventGroup, KEYBOARD_ENTER_BIT);
	xEventGroupWaitBits(xEventGroup, KEYBOARD_ENTER_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	if(sqlite3_client_get(path) == ESP_OK) { 
		sqlite3_client_delete(path); 
		sqlite3_client_get("customers"); 
	}
	ESP_LOGI(TAG, "All Finish!!");

	while(1) {
		vTaskDelay(1);
	}

}

void keyin_task(void *pvParameters)
{
	//ESP_LOGI(pcTaskGetTaskName(0), "Start");
	uint16_t c;
	while (1) {
		c = fgetc(stdin);
		if (c == 0xffff) {
			vTaskDelay(10);
			continue;
		}
		//ESP_LOGI(pcTaskGetTaskName(0), "c=%x", c);
		if (c == 0x0a) {
			ESP_LOGD(pcTaskGetTaskName(0), "Push Enter");
			xEventGroupSetBits(xEventGroup, KEYBOARD_ENTER_BIT);
		}
	}
}

void app_main()
{
	//Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
	wifi_init_sta();

	//Create EventGroup
	xEventGroup = xEventGroupCreate();

	//Check everything was created
	configASSERT( xEventGroup );

	//Start keyboard task
	xTaskCreate(keyin_task, "KEYIN", 1024*2, NULL, 2, NULL);

	//Start http task
	xTaskCreate(http_task, "HTTP", 1024*8, NULL, 2, NULL);
}
