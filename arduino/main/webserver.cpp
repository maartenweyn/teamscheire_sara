#include "webserver.h"
#include "localization.h"

#include <WiFi.h>

const char* ssid     = "ESP32-Access-Point";
const char* password = "123456789";


// Set web server port number to 80
WiFiServer server(80);

void show_page(WiFiClient client);


void webserver_setup() {
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  server.begin();
}


void webserver_check_for_client() {
//  WiFiClient client = server.available();   // Listen for incoming clients
//
//  if (client) {                             // If a new client connects,
//    Serial.println("client");
//    show_page(client);
//  } else {
//    Serial.println("no client");
//  }

  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    show_page(client);
  }
}

void print_header(WiFiClient client, int refresh) {
  // Display the HTML web page
  client.println("<!DOCTYPE html><html>");
  client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  if (refresh > 0) client.println("<meta http-equiv=\"refresh\" content=\""+ String(refresh) + "\">");
  client.println("<link rel=\"icon\" href=\"data:,\">");
  client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto;}");
  client.println(".red {color: #993300;}");
  client.println(".green {color: #99cc00;}");
  client.println("</style></head>");

  // Web Page Heading
  client.println("<body><h1>Team Scheire equestrian coach</h1>");
}
void show_page(WiFiClient client) {
  // Variable to store the HTTP request
  String header;
  
  Serial.println("New Client.");          // print a message out in the serial port
  String currentLine = "";                // make a String to hold incoming data from the client

  while (client.connected()) {            // loop while the client's connected
    if (client.available()) {             // if there's bytes to read from the client,
      char c = client.read();             // read a byte, then
      Serial.write(c);                    // print it out the serial monitor
      header += c;
      if (c == '\n') {                    // if the byte is a newline character
        // if the current line is blank, you got two newline characters in a row.
        // that's the end of the client HTTP request, so send a response:
        if (currentLine.length() == 0) {
          // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
          // and a content-type so the client knows what's coming, then a blank line:
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println("Connection: close");
          client.println();
//          
//
////          if (header.indexOf("GET /led") >= 0) {
////            int command_index = header.indexOf("/", 5);
////            int led_id = header.substring(8, command_index).toInt();
////            int value = header.substring(command_index + 1).toInt();
////            Serial.println("Led" + String(led_id) + ": " + String(value));
////          } 
//

          

          
          if (header.indexOf("GET /calib/") >= 0) {
            print_header(client, 0);
            
            //set calibration //GET /calib/1/0/300
            int anchor_id = header.substring(11, 12).toInt();
            
            int index_x = header.indexOf("/", 13);
            int x_coord = header.substring(13, index_x).toInt();
            int y_coord = header.substring(index_x+1).toInt();

            client.println("<p>New calibration for achor " + String(anchor_id) + ": (" + String(x_coord) + ", " + String(y_coord) +")</p>");
            Serial.println("New calibration for achor " + String(anchor_id) + ": (" + String(x_coord) + ", " + String(y_coord) +")");

            if (anchor_id <= 6 && anchor_id > 0) {
              node_positions[anchor_id - 1].x = x_coord;
              node_positions[anchor_id - 1].y = y_coord;
              store_calibration();
            } else {
              client.println("<p class=\"red\">Invallid anchor id</p>");
            }
          } else {
            print_header(client, 5);
          }
//
            

          //print anchor positions
          client.println("<ul>");
          for (int i = 0; i < 6; i++) {
            if (counter[i] < USE_MEASUREMENT_THRESHOLD)
              client.println("<li class=\"green\">Anchor " + String(i+1) + "(" + String(node_positions[i].x) + ", " + String(node_positions[i].y) +"): " + String(ranges[i]) +"</li>");
            else
              client.println("<li class=\"red\">Anchor " + String(i+1) + "(" + String(node_positions[i].x) + ", " + String(node_positions[i].y) +") </li>");
          }
          client.println("</ul>");
          
          client.println("<p>Position: (" + String(current_position.x) + ", " + String(current_position.y) +") Delay: " + String(last_position_counter) + "</p>");

          if (nearby_letter ==  -1) 
             client.println("<p>Letter:  None</p>");
          else
             client.println("<p>Letter:  " + String(letters[nearby_letter].letter) + "</p>");
          

          

          client.println("</body></html>");
          // The HTTP response ends with another blank line
          client.println();
         
//          
//          // Break out of the while loop
          break;
        } else { // if you got a newline, then clear currentLine
          currentLine = "";
        }
      } else if (c != '\r') {  // if you got anything else but a carriage return character,
        currentLine += c;      // add it to the end of the currentLine
      }
    } else {
      Serial.println(".");
      break;
    }
  }
  
  // Clear the header variable
  header = "";
  // Close the connection
  client.stop();
  Serial.println("Client disconnected.");
  Serial.println("");
}
