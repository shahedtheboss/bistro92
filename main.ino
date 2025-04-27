#include <WiFi.h>
#include <FirebaseESP32.h>

 



#define WIFI_SSID "CUET--Students"
#define WIFI_PASSWORD "1020304050"


#define FIREBASE_HOST "bistro-92-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "e278f2d82f863f21e64260476b3ecdc6939dce19"



FirebaseConfig config;
FirebaseAuth auth;
FirebaseData fbData;


#define BUTTON_MENU 13   
#define BUTTON_SELECT 12 
#define BUTTON_UP 14     
#define BUTTON_DOWN 27  

const char* foodItems[] = { "Burger", "Pizza", "Pasta", "Drinks" };
const int foodMenuLength = sizeof(foodItems) / sizeof(foodItems[0]);
int currentFoodSelection = 0;
int quantity = 1;
int tableNumber = 1;  // Default Table Number

int cart[foodMenuLength] = {0};

bool inFoodMenu = false;
bool inTableSelection = false;
bool inQuantityAdjust = false;
unsigned long buttonPressStart = 0;
bool buttonPressed = false;

void setup() {
    Serial.begin(115200);
    connectWiFi();
    connectFirebase();

    pinMode(BUTTON_MENU, INPUT_PULLUP);
    pinMode(BUTTON_SELECT, INPUT_PULLUP);
    pinMode(BUTTON_UP, INPUT_PULLUP);
    pinMode(BUTTON_DOWN, INPUT_PULLUP);

    showStartupMessage();
}

void loop() {
    handleButtonPress();
}

void connectWiFi() {
    Serial.print("Connecting to Wi-Fi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int retryCount = 0;
    while (WiFi.status() != WL_CONNECTED && retryCount < 15) {
        retryCount++;
        delay(1000);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWi-Fi Connected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nWi-Fi Connection Failed! Check credentials and network.");
    }
}

void connectFirebase() {
    config.host = FIREBASE_HOST;
    config.signer.tokens.legacy_token = FIREBASE_AUTH;
    
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

    Serial.println("Firebase Connected!");
}

void handleButtonPress() {
    if (digitalRead(BUTTON_MENU) == LOW) {
        if (!buttonPressed) {
            buttonPressed = true;
            buttonPressStart = millis();
        }
    } else {
        if (buttonPressed) {
            buttonPressed = false;
            unsigned long pressDuration = millis() - buttonPressStart;

            if (pressDuration >= 5000) resetCart();
            else if (pressDuration >= 2000) showCart();
            else { inTableSelection = true; showTableSelection(); }
        }
    }

    if (inTableSelection && digitalRead(BUTTON_UP) == LOW) enterFoodMenu();
    if (inFoodMenu && digitalRead(BUTTON_DOWN) == LOW) enterTableSelection();

    if (inTableSelection && digitalRead(BUTTON_SELECT) == LOW) enterTableNumberMode();

    if (inQuantityAdjust && digitalRead(BUTTON_UP) == LOW) adjustTableNumber(1);
    if (inQuantityAdjust && digitalRead(BUTTON_DOWN) == LOW) adjustTableNumber(-1);
    if (inQuantityAdjust && digitalRead(BUTTON_SELECT) == LOW) confirmTableNumber();

    if (inFoodMenu && digitalRead(BUTTON_UP) == LOW) scrollFoodMenu(-1);
    if (inFoodMenu && digitalRead(BUTTON_DOWN) == LOW) scrollFoodMenu(1);
    if (inFoodMenu && digitalRead(BUTTON_SELECT) == LOW) enterQuantityMode();

    if (inQuantityAdjust && digitalRead(BUTTON_UP) == LOW) adjustQuantity(1);
    if (inQuantityAdjust && digitalRead(BUTTON_DOWN) == LOW) adjustQuantity(-1);
    if (inQuantityAdjust && digitalRead(BUTTON_SELECT) == LOW) addToCart();
}

void showStartupMessage() {
    Serial.println("==================================");
    Serial.println("Welcome to Bistro92 Food Ordering");
    Serial.println("Press MENU to Start.");
    Serial.println("==================================");
}

void enterFoodMenu() {
    inTableSelection = false;
    inFoodMenu = true;
    showFoodMenu();
}

void enterTableSelection() {
    inFoodMenu = false;
    inTableSelection = true;
    showTableSelection();
}

void showTableSelection() {
    Serial.println("\nTable Selection:");
    Serial.println("> Select Table Number");
}

void enterTableNumberMode() {
    inTableSelection = false;
    inQuantityAdjust = true;
    showTableNumberAdjust();
}

void adjustTableNumber(int change) {
    tableNumber = max(1, tableNumber + change);
    showTableNumberAdjust();
}

void confirmTableNumber() {
    Serial.print("Table Number Set: ");
    Serial.println(tableNumber);
    inQuantityAdjust = false;
    inFoodMenu = true;
    showFoodMenu();
}

void showTableNumberAdjust() {
    Serial.println("\nAdjust Table Number:");
    Serial.print("Table No: ");
    Serial.println(tableNumber);
}

void scrollFoodMenu(int direction) {
    currentFoodSelection = (currentFoodSelection + direction + foodMenuLength) % foodMenuLength;
    showFoodMenu();
}

void showFoodMenu() {
    Serial.println("\nFood Menu:");
    for (int i = 0; i < foodMenuLength; i++) {
        Serial.print((i == currentFoodSelection) ? "> " : "  ");
        Serial.println(foodItems[i]);
    }
}

void enterQuantityMode() {
    inFoodMenu = false;
    inQuantityAdjust = true;
    showQuantityAdjust();
}

void adjustQuantity(int change) {
    quantity = max(1, quantity + change);
    showQuantityAdjust();
}

void showQuantityAdjust() {
    Serial.println("\nAdjust Quantity:");
    Serial.print("Item: ");
    Serial.println(foodItems[currentFoodSelection]);
    Serial.print("Quantity: ");
    Serial.println(quantity);
}

void showCart() {
    Serial.println("\nYour Cart:");
    Serial.print("Table No: ");
    Serial.println(tableNumber);

    bool cartIsEmpty = true;
    for (int i = 0; i < foodMenuLength; i++) {
        if (cart[i] > 0) {
            Serial.print(foodItems[i]);
            Serial.print(" x ");
            Serial.println(cart[i]);
            cartIsEmpty = false;
        }
    }

    if (cartIsEmpty) {
        Serial.println("Cart is Empty.");
    }
}

void resetCart() {
    memset(cart, 0, sizeof(cart));
    Serial.println("\nCart Reset!");
}

void addToCart() {
    cart[currentFoodSelection] += quantity;
    sendCartDataToFirebase();  // Send updated cart to Firebase
    inQuantityAdjust = false;
    inFoodMenu = true;
    quantity = 1;
    showFoodMenu();
}




void sendCartDataToFirebase() {
    FirebaseJson cartData;
    cartData.set("Table", tableNumber);  // Add Table Number

    for (int i = 0; i < foodMenuLength; i++) {
        cartData.set(foodItems[i], cart[i]);  // Store item name and quantity
    }

    Serial.print("\nSending to Firebase: ");
    cartData.toString(Serial, true);

    if (!Firebase.set(fbData, "/cart", cartData)) {
        Serial.print("Firebase Error: ");
        Serial.println(fbData.errorReason());
    } else {
        Serial.println("Cart Data Sent Successfully!");
    }
}
