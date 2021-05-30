//Libraries
#include <ArduinoJson.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <Wire.h>

char receivedData[30]; // this variable is used to store the received data
byte nData = 0;

//Process Secret Data
char ssid[] = ""; // your network SSID (name)
char pass[] = ""; // your network password (use for WPA, or use as key for WEP)
char token[] = ""; // your lichess API token

//user input variables and constants
char uci[] = { ' ', ' ', ' ', ' ', 0 };
boolean se = false;
String clockTime = "120";
int clockIncrement = 30;
String corrUser;
int mod = '0';

// Wifi variables
int keyIndex = 0; // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;
char server[] = "lichess.org"; // name address for lichess (using DNS)
WiFiSSLClient client; // WIFISSLClient always connects via SSL (port 443 for https)

//lichess variables
const char* username;
const char* currentGameID;
const char* previousGameID;
const char* lastMove;
const char* myColour;
const char* opponentName;
const char* opponentColour;
const char* moveError;
const char* winner;
const char* endStatus;
const char* challengeID;
const char* preMove;
const char* fen;
String fen2 = "                                                                                ";
String selectedColour;
String fens;
String fenan = "                                                                                ";
String mov;
String proof = "a"; //***********************************
String lan = "      ";
boolean myTurn = false;
boolean moveSuccess = false;
boolean gameInit = true;
boolean challengeSent = false;
const int delayTime = 100; //the time to wait for lichess.org to respond to an http request

// loop variables
boolean MODE0 = true;
boolean MODE1 = false;
int modeIndex = 0;
int ends = 0;
int lenfen = 0;
int c = 0;
int posfen = 0;

// function prototypes
void printWiFiStatus();
void processHTTP();
void moveInput();
void modeSelect();
void alphaNumSelect();
void clockSelect();
void colourSelect();
void requestEvent();
void castlingVerification();
void receivedData(int howMany);
void changeFen();
void DataRequest();

void setup()
{
    //Initialize serial and wait for port to open:
    Serial.begin(9600);

    Wire.begin(0x14); //set i2c as slave
    Wire.onRequest(DataRequest);

    delay(1000);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }
    // check for the WiFi module:
    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("Communication with WiFi module failed!");
        // don't continue
        while (true)
            ;
    }
    String fv = WiFi.firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
        Serial.println("Please upgrade the firmware");
    }
    // attempt to connect to WiFi network:

    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid);
        status = WiFi.begin(ssid, pass);

        // wait 10 seconds for connection:
        delay(10000);
    }
    Serial.println("Connected to wifi");

    printWiFiStatus();

    Serial.println("\nStarting connection to server...");
    // if you get a connection, report back via serial:
    if (client.connect(server, 443)) {
        Serial.println("connected to server in setup");
        client.println("GET /api/account HTTP/1.1");
        client.println("Host: lichess.org");

        client.print("Authorization: Bearer ");
        client.println(token);
        delay(delayTime); //delay to allow a response

        processHTTP();

        DynamicJsonDocument doc(2048);

        DeserializationError error = deserializeJson(doc, client);
        if (error) {
            // this is due to an error in the HTTP request
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            return;
        }
        // Extract values
        
        // lichess username
        username = doc["username"];
        //close request
        client.println("Connection: close");
        client.println();
        if (username != NULL) {
            Serial.print("connected to the user: ");
            Serial.println(username);
            delay(1000);
        }
        //we were unable to connect to the server
    } else {
        Serial.println("Server failed on setup....");
    }
}

void loop()
{
    //
    if (MODE0 == true) {
        // Make a HTTP request:
        if (client.connect(server, 443)) {
            Serial.println("connected to server for ongoing game");
            //keep the request so lichess knows you are there
            client.println("GET /api/account/playing HTTP/1.1");
            client.println("Host: lichess.org");
            client.print("Authorization: Bearer ");
            client.println(token);
            delay(delayTime); //delay to allow a response
            processHTTP();
            DynamicJsonDocument doc(2048);
            DeserializationError error = deserializeJson(doc, client);
            if (error) {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.f_str());
                Serial.println("HTTP error MODE0 INIT");
                return;
            }
            client.stop();
            // Extract values
            JsonObject nowPlaying_0 = doc["nowPlaying"][0];
            JsonObject nowPlaying_0_opponent = nowPlaying_0["opponent"];
            //Serial.println(F("Response:"));**********************************
            currentGameID = nowPlaying_0["gameId"];
            Serial.print("current game ID: ");
            Serial.println(currentGameID);
            // if there is an ongoing game
            if (currentGameID != NULL) {
                // extract information from the output json

                myTurn = nowPlaying_0["isMyTurn"];
                lastMove = nowPlaying_0["lastMove"];
                myColour = nowPlaying_0["color"];
                opponentName = nowPlaying_0_opponent["username"];
                fen = nowPlaying_0["fen"];

                if (strcmp(myColour, "black") == 0) {
                    opponentColour = "white";
                } else {
                    opponentColour = "black";
                }

                if (strcmp(currentGameID, previousGameID) != 0) {
                    gameInit = true;
                }

                if (gameInit == true) {

                    // if a user has queued a premove there is no time to delay
                    if (preMove == NULL) {
                        delay(500);
                    }
                    Serial.print("Your opponent's name: ");
                    Serial.println(opponentName); //************************************************************************************

                    gameInit = false;
                    // if the user makes a premove and is playing white, the last move needs to be displayed
                    if (preMove != NULL and strcmp(myColour, "black") == 0) {
                        Serial.print("white: ");
                        Serial.println(lastMove);

                    } else {
                        Serial.print("Your color: ");
                        Serial.println(myColour);
                        previousGameID = nowPlaying_0["gameId"];
                        // reset correspondance variables
                        challengeSent = false;
                        // if a user has queued a premove there is no time to delay
                        if (preMove == NULL) {
                            delay(1000);
                        }
                    }
                }
                // send a POST request with the chosen move if it is your turn
                if (myTurn == true) {
                    // user inputs a move (unless its a premove)
                    proof = lastMove;
                    changeFen();
                    Serial.println("");
                    Serial.println("It's your turn");
                    if (preMove == NULL) {
                        Serial.print("Opponent's movement is ");
                        Serial.print(opponentColour);
                        Serial.print(": ");
                        castlingVerification();
                        posfen = 0;
                        switch (proof[3]) {
                        case '7':
                            posfen = posfen + 8;
                            break;
                        case '6':
                            posfen = posfen + 16;
                            break;
                        case '5':
                            posfen = posfen + 24;
                            break;
                        case '4':
                            posfen = posfen + 32;
                            break;
                        case '3':
                            posfen = posfen + 40;
                            break;
                        case '2':
                            posfen = posfen + 48;
                            break;
                        case '1':
                            posfen = posfen + 56;
                            break;
                        }
                        switch (proof[2]) {
                        case 'b':
                            posfen = posfen + 1;
                            break;
                        case 'c':
                            posfen = posfen + 2;
                            break;
                        case 'd':
                            posfen = posfen + 3;
                            break;
                        case 'e':
                            posfen = posfen + 4;
                            break;
                        case 'f':
                            posfen = posfen + 5;
                            break;
                        case 'g':
                            posfen = posfen + 6;
                            break;
                        case 'h':
                            posfen = posfen + 7;
                            break;
                        }
                        if (fenan[posfen] != ' ') {
                            lan[0] = fen2[posfen];
                            lan[1] = proof[0];
                            lan[2] = proof[1];
                            lan[3] = 'x';
                            lan[4] = proof[2];
                            lan[5] = proof[3];
                        } else {
                            lan[0] = fen2[posfen];
                            lan[1] = proof[0];
                            lan[2] = proof[1];
                            lan[3] = '-';
                            lan[4] = proof[2];
                            lan[5] = proof[3];
                        }
                        Serial.println(lan);
                        Serial.print("the current positions are: ");
                        Serial.println(fen2);
                        Serial.print("the previous positions are: ");
                        Serial.println(fenan);
                        Serial.print("input: ");
                        Serial.write(0x0d); //rewrites the input of the serial port to 0's
                        while (se == false) {
                            moveInput();
                        }
                        se = false;
                    }
                    Serial.println("Sending move information...");
                    if (client.connect(server, 443)) {
                        Serial.println("connected to server to send move information");
                        client.print("POST /api/board/game/");
                        client.print(currentGameID);
                        if (preMove != NULL) {
                            client.print("/move/");
                            client.print(preMove);
                            preMove = NULL;
                        } else {
                            if (strcmp(uci, "g g") == 0) {
                                client.print("/resign");
                            } else if (strcmp(uci, "d d") == 0) {
                                client.print("/draw/yes");
                            } else {
                                client.print("/move/");
                                client.print(uci);
                            }
                        }
                        client.println(" HTTP/1.1");
                        client.println("Host: lichess.org");
                        client.print("Authorization: Bearer ");
                        client.println(token);
                        delay(delayTime); //delay to allow response time
                        processHTTP(); // if the move is invalid it will get picked up in this function as a 400 Bad request
                        StaticJsonDocument<48> doc;
                        DeserializationError error = deserializeJson(doc, client);
                        if (error) {
                            Serial.print(F("deserializeJson() failed: "));
                            Serial.println(error.f_str());
                            Serial.println("HTTP failed...");
                            return;
                        }
                        client.stop();
                        // Chec if the movement was valid
                        moveSuccess = doc["ok"];
                        if (moveSuccess == true) {
                            Serial.println("move success!");
                        } else {
                            Serial.println("Invalid move");
                        }
                    }
                    fenan = fen2;
                    // if it's not my turn
                } else {
                    proof = lastMove;
                    Serial.println("");
                    Serial.print("Your last movement was: ");

                    castlingVerification();
                    Serial.println(proof);

                    Serial.print("The current positions are: ");
                    changeFen();
                    Serial.println(fen2);

                    Serial.print(opponentName);
                    Serial.println("'s turn");
                    Serial.println("waiting for opponent turn...");
                }
                fenan = fen2;
                /// if there is no ongoing game
            } else {
                //reset MODE0 if we aren't waiting for a challenge accept
                if (challengeSent != true) {
                    MODE0 = false;
                }
            }
        }
    }
    //take user to main menu to choose between MODE1 or MODE2
    else {
        if (gameInit == false) {
            // This happens when it connects to a finished game and it requests the status
            if (client.connect(server, 443)) {
                Serial.println("connected to server for finished game");
                Serial.println(previousGameID);
                client.print("GET /api/board/game/stream/");
                client.print(previousGameID);
                client.println(" HTTP/1.1");
                client.println("Host: lichess.org");
                client.print("Authorization: Bearer ");
                client.println(token);
                delay(delayTime); //delay to allow a response
                processHTTP();
                while (client.available()) {
                    char p = client.peek();
                    if (p == '{') {
                        break;
                    } else {
                        char c = client.read();
                        Serial.print(c);
                    }
                }
                StaticJsonDocument<1536> doc;
                DeserializationError error = deserializeJson(doc, client);
                if (error) {
                    Serial.print(F("deserializeJson() failed: "));
                    Serial.println(error.f_str());
                    Serial.println("HTTP failed at end game");
                    return;
                }
                //close request
                client.println("Connection: close");
                client.println();
                client.stop();
                // Extract values
                JsonObject state = doc["state"];
                winner = state["winner"];
                endStatus = state["status"];
                Serial.print("The winner is: ");
                Serial.println(winner);
                Serial.println(endStatus);
                while (se == false) {
                    if (Serial.available() > 0) {
                        ends = Serial.read();
                        ends = ends - 47;
                        if (ends == 1) {
                            se = true;
                            Serial.write(0x0d);
                            delay(100);
                        }
                    }
                }
                se = false;
                gameInit = true;
            }
        }

        Serial.println("Create game:");
        while (se == false) {
            modeSelect();
        }
        se = false;
        if (MODE1 == true) { // Mode 1 is to challenge another user

            Serial.println("Enter username:");

            while (se == false) {
                alphaNumSelect();
            }
            se = false;

            Serial.println("Sending challenge...");
            delay(500);

            if (client.connect(server, 443)) {
                Serial.println("connected to server to send challenge");
                client.print("POST /api/challenge/");
                client.print(corrUser);
                client.println(" HTTP/1.1");
                client.println("Host: lichess.org");
                client.print("Authorization: Bearer ");
                client.println(token);
                delay(delayTime); //delay to allow a response
                processHTTP();
                StaticJsonDocument<1536> doc;
                DeserializationError error = deserializeJson(doc, client);
                if (error) {
                    Serial.print(F("deserializeJson() failed: "));
                    Serial.println(error.f_str());
                    Serial.println("HTTP failed at corr create");
                    return;
                }
                //close request
                client.println("Connection: close");
                client.println();
                client.stop();
                // Extract values
                JsonObject challenge = doc["challenge"];
                challengeID = challenge["id"];
                // This is returned if the challenge ID was sent succesfully
                if (challengeID != NULL) {
                    Serial.println("Challenge sent ID: ");
                    Serial.print(challengeID);
                    Serial.println("");
                    challengeID = NULL;
                    challengeSent = true;
                    MODE0 = true;
                }
            }
        } else {
            // MODE 2: RANDOM MATCH
            // PLAYER CAN INSTANTLY START A CASUAL MATCH WITH A RANDOM  OPPONENT
            // USER DEFINES TIME AND CLOCK INCREMENT
            // ONE MATCH IS FOUND RETURN TO MODE 0
            while (se == false) {
                clockSelect();
            }
            se = false;
            while (se == false) {
                colourSelect();
            }
            se = false;
            Serial.print("Input: ");
            while (se == false) {
                moveInput();
            }
            se = false;
            preMove = uci;
            String postData;
            String stringOne = String("time=") + clockTime;
            String stringTwo = String("&increment=") + clockIncrement;
            String stringThree = String("&color=") + selectedColour;
            postData = stringOne + stringTwo + stringThree;
            Serial.println(postData);
            if (client.connect(server, 443)) {
                Serial.println("connected to server to send challenge");
                client.println("POST /api/board/seek HTTP/1.1");
                client.println("Host: lichess.org");
                client.print("Authorization: Bearer ");
                client.println(token);
                client.println("Content-Type: application/x-www-form-urlencoded");
                client.print("Content-Length: ");
                client.println(postData.length());
                client.println();
                client.println(postData);
                processHTTP();
                boolean game_found = false;
                while (game_found == false) {
                    Serial.println("Searching...");
                    if (client.println() == 0) {
                        game_found = true;
                        Serial.println("Game found.");
                    }
                }
            }
            // Return to MODE0 and connect to the game that we just found
            MODE0 = true;
        }
    }
}

//  -------------------------Functions----------------------------   //

void printWiFiStatus()
{
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your board's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}

void processHTTP()
{
    // this function processes http data before it is read by arduino json
    if (client.println() == 0) {
        Serial.println(F("Failed to send request"));
        return;
    }
    // Check HTTP status
    char status[32] = { 0 };
    client.readBytesUntil('\r', status, sizeof(status));
    // It should be "HTTP/1.0 200 OK"
    if (strcmp(status + 9, "200 OK") != 0) {
        Serial.print(F("Unexpected response: "));
        Serial.println(status);
        if (strcmp(status + 9, "400 Bad Request") == 0) {
            delay(500);
        } else {
            return;
        }
    }
    // Skip HTTP headers
    char endOfHeaders[] = "\r\n\r\n";
    if (!client.find(endOfHeaders)) {
        Serial.println(F("Invalid response"));
        return;
    }
}

//// This function allows to enter a movement in UCI format
// "d d " to offer draw
// "g g " to give up
void moveInput()
{

    if (Serial.available() > 0) {
        // read the incoming byte:
        mov = Serial.readString();
        uci[0] = mov[0];
        uci[1] = mov[1];
        uci[2] = mov[2];
        uci[3] = mov[3];
        Serial.println(uci);
        se = true;
        delay(100);
    }
}

void modeSelect()
{
    Serial.println("Select mode 1 to correspondence chess and 2 to random mode: ");
    while (Serial.available() == 0) {
    }
    if (Serial.available() > 0) {
        // read the incoming byte:
        mod = Serial.read();
        mod = mod - 48;

        if (mod == 1) {
            MODE1 = true;
        }
        if (mod == 2) {
            MODE1 = false;
        }
        se = true;
        Serial.write(0x0d);
        delay(500);
    }
}

void alphaNumSelect()
{
    // This function allows users to input an opponents name for correspondance game

    if (Serial.available() > 0) {
        // read the incoming byte:
        corrUser = Serial.readString();
        Serial.println(corrUser);
        se = true;
        Serial.write(0x0d);
        delay(100);
    }
}

void clockSelect()
{
    // this functions determines the clock time, up and down change the game time, left and right change the increment
    if (Serial.available() > 0) {
        // read the incoming byte:
        clockTime = Serial.readString();
        se = true;
        Serial.write(0x0d);
    }
    Serial.print("time: ");
    Serial.println(clockTime);
}

void colourSelect()
{
    // this functions determines the clock time, up and down change the game time, left and right change the increment
    Serial.println("writre 'white' or 'black' to select your color piece");
    if (Serial.available() > 0) {
        // read the incoming byte:
        selectedColour = Serial.readString();
        Serial.print("time: ");
        Serial.println(clockTime);
        se = true;
        Serial.write(0x0d);
    }
    delay(100);
}

void requestEvent() 
{
    Wire.write(lastMove);
}

void castlingVerification()
{

    if (proof == "e8h8" || proof == "e1h1") {
        proof = "O-O";
    } else if (proof == "e8a8" || proof == "e1a1") {
        proof = "O-O-O";
    }
}

void receivedData(int howMany)
{
    nData = 0;
    while (Wire.available() > 0) {
        char dato = Wire.read();

        if (dato == 'a') {
            while (Wire.available() > 0) {
                char dato = Wire.read();
                if (dato != '\0') {
                    ssid[nData] = dato; //it concatenates the character into ssid
                    nData++;
                } else
                    break;
            }
        }

        if (dato != '\0') {
            pass[nData] = dato; //it concatenates the character into pass
            nData++; 
        } else
            break;
    }
}

void changeFen()
{
    fens = fen;
    c = 0;
    lenfen = fens.length();
    for (int i = 0; i <= lenfen; i++) {

        switch (fen[i]) {
        case '1':
            fen2[c] = ' ';
            c = c + 1;
            break;
        case '2':
            for (int y = 0; y <= 1; y++) {
                fen2[c] = ' ';
                c = c + 1;
            }
            break;
        case '3':
            for (int y1 = 0; y1 <= 2; y1++) {
                fen2[c] = ' ';
                c = c + 1;
            }
            break;
        case '4':
            for (int y2 = 0; y2 <= 3; y2++) {
                fen2[c] = ' ';
                c = c + 1;
            }
            break;
        case '5':
            for (int y3 = 0; y3 <= 4; y3++) {
                fen2[c] = ' ';
                c = c + 1;
            }
            break;
        case '6':
            for (int y = 0; y <= 5; y++) {
                fen2[c] = ' ';
                c = c + 1;
            }
            break;
        case '7':
            for (int y = 0; y <= 6; y++) {
                fen2[c] = ' ';
                c = c + 1;
            }
            break;
        case '8':
            for (int y7 = 0; y7 <= 7; y7++) {
                fen2[c] = ' ';
                c = c + 1;
            }
            break;
        case '/':
            break;
        default:
            fen2[c] = fen[i];
            c = c + 1;
            break;
        }
    }
}
void DataRequest()
{
    for (int g = 0; g <= 5; g++) {
        Wire.write(lan[g]);
    }
}