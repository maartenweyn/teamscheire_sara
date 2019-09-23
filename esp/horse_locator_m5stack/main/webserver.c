// /* Standard includes. */
// #include "string.h"
// #include "esp_err.h"
// /* lwIP core includes */
// #include "lwip/opt.h"
// #include "lwip/sockets.h"
// #include <netinet/in.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_vfs_fat.h"
// #include "driver/sdmmc_host.h"
// #include "driver/sdmmc_defs.h"
// #include "sdmmc_cmd.h"
// /* Utils includes. */
// #include "esp_log.h"
// #include "event.h"
// #include "http.h"
// #include "webserver.h"
// #include "cJSON.h"
// #include <dirent.h>

#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>

#include <http_server.h>

#include "localization.h"
#include "app_config.h"


#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include <esp_log.h>
#define TAG "webserver:"


// #define GPIO_OUTPUT_IO_0    0//16
// #define GPIO_OUTPUT_PIN_SEL  ((1<<GPIO_OUTPUT_IO_0))


// static uint32_t http_url_length=0;
// static uint32_t http_body_length=0;

// static char* http_body=NULL;
// static char* http_url=NULL;

// static int32_t socket_fd, client_fd;
// #define RES_HEAD "HTTP/1.1 200 OK\r\nContent-type: %s\r\nTransfer-Encoding: chunked\r\n\r\n"
// static char chunk_len[15];

static char read_buf[1024];


static void print_header(int refresh, httpd_req_t *req) {
  // Display the HTML web page
	memset(read_buf, 0, 1024);

	strcpy(read_buf, "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");

	if (refresh > 0) {
		char temp[10];
		sprintf(temp, "%d", refresh);
  	    strcat(read_buf, "<meta http-equiv=\"refresh\" content=\"");
		strcat(read_buf, temp);
		strcat(read_buf, "\">");
	}

  strcat(read_buf, "<link rel=\"icon\" href=\"data:,\">");
  strcat(read_buf, "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto;}");
  strcat(read_buf, ".red {color: #993300;}");
  strcat(read_buf, ".green {color: #99cc00;}");
  strcat(read_buf, "</style></head>");

  // Web Page Heading
  strcat(read_buf, "<body><h1>Team Scheire equestrian coach</h1>");

	//ESP_LOGD(TAG,"HEADER %s", read_buf);

   
   httpd_resp_set_type     (req, HTTPD_TYPE_TEXT); 
   httpd_resp_send_chunk(req, read_buf, strlen(read_buf));
}

void print_main_content(httpd_req_t *req) {
		memset(read_buf, 0, 1024);
		char temp[100];
		
		strcpy(read_buf, "<h2>Measuruments</h2>");

		strcat(read_buf, "<ul>");
		for (int i = 0; i < 6; i++) {
			if (meas_counter[i] < USE_MEASUREMENT_THRESHOLD)
				sprintf(temp, "<li class=\"green\">Anchor %d (%d, %d): %d cm</li>", i+1, app_config.node_positions[i].x, app_config.node_positions[i].y, meas_ranges[i]);
			else
				sprintf(temp, "<li class=\"red\">Anchor %d (%d, %d)</li>", i+1, app_config.node_positions[i].x, app_config.node_positions[i].y);
			strcat(read_buf, temp);
		}
		strcat(read_buf, "</ul>");

		strcat(read_buf, "<h2>Position</h2>");
		sprintf(temp, "<p>Position: (%d, %d)<br>Delay: %d</p>",  current_position.x, current_position.y, last_position_counter);
		strcat(read_buf, temp);

		if (nearby_letter ==  -1) 
		{
			strcat(read_buf, "<p>Letter:  None</p>");
		} else {
			sprintf(temp, "<p>Letter:  %c</p>", app_config.letters[nearby_letter].letter);
			strcat(read_buf, temp);
		}

		strcat(read_buf, "</body></html>");

		//ESP_LOGD(TAG,"CONTENT %s", read_buf);

		httpd_resp_send_chunk(req, read_buf, strlen(read_buf));
}

void print_config_content(httpd_req_t *req) {
		memset(read_buf, 0, 1024);
		char temp[200];
		
		strcpy(read_buf, "<h2>Configuration</h2>");

        strcat(read_buf, "<form \"config\" method=\"get\">");
        strcat(read_buf, "Wi-Fi SSID<br>");
        sprintf(temp, "<input type=\"text\" name=\"wifi_ssid\" value=\"%s\">", app_config.wifi_ssid);
        strcat(read_buf, temp);
        strcat(read_buf, "");
        strcat(read_buf, "<br>");
        strcat(read_buf, "Wi-Fi password:<br>");
        sprintf(temp, "<input type=\"password\" name=\"wifi_passwd\" value=\"%s\">", app_config.wifi_passwd);
        strcat(read_buf, temp);
        strcat(read_buf, "<br>");
        strcat(read_buf, "Nearby threshold (cm):<br>");
        sprintf(temp, "<input type=\"text\" name=\"nearby_threshold\" value=\"%d\">", app_config.nearby_threshold);
        strcat(read_buf, temp);
        strcat(read_buf, "<br>");
        for (int i = 0; i < 6; i++) {
			sprintf(temp, "Node %d x / y (cm):<br><input type=\"text\" name=\"node_positions_%d_x\" value=\"%d\"><input type=\"text\" name=\"node_positions_%d_y\" value=\"%d\"><br>", i+1, i+1, app_config.node_positions[i].x,  i+1, app_config.node_positions[i].y);
            strcat(read_buf, temp);
		}

        httpd_resp_send_chunk(req, read_buf, strlen(read_buf));
        ESP_LOGD(TAG,"CONTENT (%d) %s", strlen(read_buf), read_buf);
        memset(read_buf, 0, 1024);
  
        strcat(read_buf, "Field margin (cm):<br>");
        sprintf(temp, "<input type=\"text\" name=\"field_size_margin\" value=\"%d\">", app_config.field_size_margin);
        strcat(read_buf, temp);
        strcat(read_buf, "<br>");
        strcat(read_buf, "Field Size x / y (cm):<br>");
        sprintf(temp, "<input type=\"text\" name=\"field_size_x\" value=\"%d\">", app_config.field_size.x);
        strcat(read_buf, temp);
        sprintf(temp, "<input type=\"text\" name=\"field_size_y\" value=\"%d\">", app_config.field_size.y);
        strcat(read_buf, temp);
        strcat(read_buf, "<br>");

        httpd_resp_send_chunk(req, read_buf, strlen(read_buf));
        ESP_LOGD(TAG,"CONTENT (%d) %s", strlen(read_buf), read_buf);
        memset(read_buf, 0, 1024);

        for (int i = 0; i < NR_OF_LETTERS; i++) {
			sprintf(read_buf, "Letter %c x / y (cm):<br><input type=\"text\" name=\"letter_%d_x\" value=\"%d\"><input type=\"text\" name=\"letter_%d_y\" value=\"%d\"><br>", app_config.letters[i].letter, i+1, app_config.letters[i].x, i+1, app_config.letters[i].y);
            //strcat(read_buf, temp);
            httpd_resp_send_chunk(req, read_buf, strlen(read_buf));
            ESP_LOGD(TAG,"CONTENT (%d) %s", strlen(read_buf), read_buf);
            memset(read_buf, 0, 1024);
		}

        strcat(read_buf, "<br><br>");
        strcat(read_buf, "<input type=\"submit\" value=\"Submit\">");
        strcat(read_buf, "</form>");
		strcat(read_buf, "</body></html>");

		ESP_LOGD(TAG,"CONTENT (%d) %s", strlen(read_buf), read_buf);

		httpd_resp_send_chunk(req, read_buf, strlen(read_buf));
}

// static void return_file(char* filename){
// 	uint32_t r;
//   	FILE* f = fopen(filename, "r");
//   	if(f==NULL){
//   		ESP_LOGE(TAG,"cannot not find the file %s", filename);
//   		return;
//   	}
//   	while(1){
//     	r=fread(read_buf,1,1024,f);
//     	if(r>0){
//     		sprintf(chunk_len,"%x\r\n",r);
//     		write(client_fd, chunk_len, strlen(chunk_len));
// 	    	//printf("%s",dst_buf);
// 	    	write(client_fd, read_buf, r);
// 	    	write(client_fd, "\r\n", 2);
//     	}else
//     		break;
//     }
//     fclose(f);
//   	chunk_end(client_fd);
// }



esp_err_t index_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }



    print_header(5, req);
    print_main_content(req);

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

esp_err_t config_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    print_header(0, req);

    memset(read_buf, 0, 1024);
    strcpy(read_buf, "<a href=\"/\">Home</a>");

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    bool safe_config = false;
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGD(TAG, "Found URL query => %s", buf);
            char param[32];
            // wifi
            if (httpd_query_key_value(buf, "wifi_ssid", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => wifi_ssid=%s", param);
                strcpy(app_config.wifi_ssid, param);
                safe_config = true;
            }
            if (httpd_query_key_value(buf, "wifi_passwd", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => wifi_passwd=%s", param);
                strcpy(app_config.wifi_passwd, param);
                safe_config = true;
            }
            //node positions
            for (int i = 0; i < 6; i++) {
                char par_name[24];
			    sprintf(par_name, "node_positions_%d_x", i+1);
                if (httpd_query_key_value(buf, par_name, param, sizeof(param)) == ESP_OK) {
                    ESP_LOGI(TAG, "Found URL query parameter => %s=%s", par_name, param);
                    app_config.node_positions[i].x = atoi(param);
                    safe_config = true;
                }
                sprintf(par_name, "node_positions_%d_y", i+1);
                if (httpd_query_key_value(buf, par_name, param, sizeof(param)) == ESP_OK) {
                    ESP_LOGI(TAG, "Found URL query parameter => %s=%s", par_name, param);
                    app_config.node_positions[i].y = atoi(param);
                    safe_config = true;
                }
		    }

            //nearby_threshold
            if (httpd_query_key_value(buf, "nearby_threshold", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => %s=%s", "nearby_threshold", param);
                app_config.nearby_threshold = atoi(param);
                safe_config = true;
            }

            //field_size
            if (httpd_query_key_value(buf, "field_size_x", param, sizeof(param)) == ESP_OK) {
                    ESP_LOGI(TAG, "Found URL query parameter => %s=%s", "field_size_x", param);
                    app_config.field_size.x = atoi(param);
                    safe_config = true;
            }
            if (httpd_query_key_value(buf, "field_size_y", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => %s=%s", "field_size_y", param);
                app_config.field_size.y = atoi(param);
                safe_config = true;
            }

            //field_size_margin
            if (httpd_query_key_value(buf, "field_size_margin", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => %s=%s", "field_size_margin", param);
                app_config.field_size_margin = atoi(param);
                safe_config = true;
            }

            //letters
            for (int i = 0; i < NR_OF_LETTERS; i++) {
                char par_name[24];
			    sprintf(par_name, "letter_%d_x", i+1);
                if (httpd_query_key_value(buf, par_name, param, sizeof(param)) == ESP_OK) {
                    ESP_LOGI(TAG, "Found URL query parameter => %s=%s", par_name, param);
                    app_config.letters[i].x = atoi(param);
                    safe_config = true;
                }

                sprintf(par_name, "letter_%d_y", i+1);
                if (httpd_query_key_value(buf, par_name, param, sizeof(param)) == ESP_OK) {
                    ESP_LOGI(TAG, "Found URL query parameter => %s=%s", par_name, param);
                    app_config.letters[i].y = atoi(param);
                    safe_config = true;
                }
            }
            
            if (safe_config) save_config();
            
        }
        free(buf);
    }


    print_config_content(req);

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

esp_err_t calib_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;
		
    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    print_header(0, req);

    memset(read_buf, 0, 1024);
    strcpy(read_buf, "<a href=\"/\">Home</a>");

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[32];
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "id", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => id=%s", param);
                int anchor_id = atoi(param);
                if (httpd_query_key_value(buf, "x", param, sizeof(param)) == ESP_OK) {
                    ESP_LOGI(TAG, "Found URL query parameter => x=%s", param);
                    int x_coord = atoi(param);
                    if (httpd_query_key_value(buf, "y", param, sizeof(param)) == ESP_OK) {
                        ESP_LOGI(TAG, "Found URL query parameter => y=%s", param);
                        int y_coord = atoi(param);
                        if (anchor_id <= 6 && anchor_id > 0) {
                            app_config.node_positions[anchor_id - 1].x = x_coord;
                            app_config.node_positions[anchor_id - 1].y = y_coord;
                            char temp[50];
                            sprintf(temp, "<p class=\"green\'>Anchor %d adapted</p>", anchor_id);
                            strcat(read_buf, temp);
                            save_config();
                        } else {
                            strcat(read_buf, "<p class=\"red\">Invallid anchor id</p>");
                        }
                    }
                }
            }
        }
        free(buf);
    }
		
    httpd_resp_send_chunk(req, read_buf, strlen(read_buf));

    print_main_content(req);

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_handler,
    .user_ctx  = NULL
};

// httpd_uri_t favicon = {
//     .uri       = "/favicon.ico",
//     .method    = HTTP_GET,
//     .handler   = favicon_handler,
//     .user_ctx  = NULL
// };

httpd_uri_t calib = {
    .uri       = "/calib",
    .method    = HTTP_GET,
    .handler   = calib_handler,
    .user_ctx  = NULL
};

httpd_uri_t config_config = {
    .uri       = "/config",
    .method    = HTTP_GET,
    .handler   = config_handler,
    .user_ctx  = NULL
};

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &index_uri);
        //httpd_register_uri_handler(server, &favicon);
        httpd_register_uri_handler(server, &calib);
        httpd_register_uri_handler(server, &config_config);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    httpd_handle_t *server = (httpd_handle_t *) ctx;

    switch(event->event_id) {
    case SYSTEM_EVENT_AP_STACONNECTED:
        if (*server == NULL) {
            *server = start_webserver();
        }
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        if (*server) {
                stop_webserver(*server);
                *server = NULL;
            }
        break;
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
        ESP_ERROR_CHECK(esp_wifi_connect());
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
  ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
        ESP_LOGI(TAG, "Got IP: '%s'",
                ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));

        /* Start the web server */
        if (*server == NULL) {
            *server = start_webserver();
        }
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
        ESP_ERROR_CHECK(esp_wifi_connect());

        /* Stop the web server */
        if (*server) {
            stop_webserver(*server);
            *server = NULL;
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}


void initialise_wifi(void *arg) {

	tcpip_adapter_init();

	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, arg));


	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM)); 


	wifi_config_t wifi_config;
	memset(&wifi_config,0,sizeof(wifi_config));

	memcpy(wifi_config.ap.ssid, app_config.wifi_ssid, strlen(app_config.wifi_ssid));
	wifi_config.ap.ssid_len=strlen(app_config.wifi_ssid);

	wifi_config.ap.max_connection=3;
	memcpy(wifi_config.ap.password, app_config.wifi_passwd, strlen(app_config.wifi_passwd));
	wifi_config.ap.authmode=WIFI_AUTH_WPA_WPA2_PSK;

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(TAG, "wifi_init_softap finished.SSID:%s password:%s \n",\
					app_config.wifi_ssid, app_config.wifi_passwd);

}