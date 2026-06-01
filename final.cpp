#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <Windows.h>
#include <WinInet.h>

//built in windows internet library (lets us do web requests)
#pragma comment(lib, "wininet.lib")

using namespace std;

//this structure used by the vector groups an item's data together into a single entry
struct PokemonItem {
    string name;
    string effect;
    int quantity; 
};

//removes carriage returns and forces lowercase for clean input
string cleanString(const string& str) {
    string cleaned;
    for (char c : str) {
        if (c == ' ') cleaned += '-';
        else if (c != '\r' && c != '\n')
            cleaned += tolower(c);
    }
    return cleaned;
}

string fetchItemDescription(const string& itemName) {
    string resultBody = "";
    
    //init native windows internet sessions
    HINTERNET hInternet = InternetOpenA("PokeAgent", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return "";

    //connect to pokeapi
    HINTERNET hConnect = InternetConnectA(hInternet, "pokeapi.co", INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return "";
    }

    //opens request channel
    string path = "/api/v2/item/" + itemName + "/?language=en";
    HINTERNET hRequest = HttpOpenRequestA(hConnect, "GET", path.c_str(), NULL, NULL, NULL, INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD, 0);
    if (!hRequest) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return "";
    }

    //send request
    if (HttpSendRequestA(hRequest, NULL, 0, NULL, 0)) {
        char buffer[1024];
        DWORD bytesRead = 0;
        
        //continuously read web data
        while (InternetReadFile(hRequest, &buffer[0], sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            resultBody += buffer;
        }
    }

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    if (!resultBody.empty() && resultBody.find("Not Found") == string::npos) {
        try {
            json data = json::parse(resultBody);

            //type checking and string conversion to ensure english results
            if (data.contains("effect_entries")) {
                for (const auto& entry : data["effect_entries"]) {
                    if (entry.is_object() && 
                        entry.contains("language") &&
                        entry["language"].is_object() &&
                        entry["language"].contains("name") &&
                        entry["language"]["name"].is_string() &&
                        entry["language"]["name"].get<string>() == "en" &&
                        entry.contains("short_effect") &&
                        entry["short_effect"].is_string()) {

                        return entry["short_effect"].get<string>();
                    }
                }
            }
        } catch (const json::exception& e) {
            return "";
        }
    }
    return ""; 
}

int main() {

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    //vector that stores collection of structure elements
    vector<PokemonItem> organizerInventory;
    int choice = 0;

    while (true) {
        cout << "\n1. Add/Stack Item\n2. View Inventory\n3. Update Quantity\n4. Delete Item\n5. Exit\n";
        cout << "Enter choice 1-5: ";
        
        //catch and fix menu typing errors
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(1000, '\n');
            continue;
        }
        cin.ignore(1000, '\n'); //clears leftover newline memory (important because this will mess with other inputs)

        //exit program
        if (choice == 5) {
            cout << "Exiting program" << endl;
            break;
        }

        //option 1----------------------------------------------------------------------------------------------------------
        if (choice == 1) {
            string rawItemName;
            int qty;
            cout << "Enter item name: ";
            cin >> rawItemName;
            cout << "Enter quantity: ";
            cin >> qty;
            cin.ignore(1000, '\n');

            //pass string through cleaner upper
            string itemName = cleanString(rawItemName);

            //checks if item already exists locally
            bool alreadyExists = false;
            for (size_t i = 0; i < organizerInventory.size(); i++) {
                if (organizerInventory[i].name == itemName) {
                    organizerInventory[i].quantity += qty; 
                    cout << "New total: " << organizerInventory[i].quantity << endl;
                    alreadyExists = true;
                    break;
                }
            }

            //if the item doesn't exist locally, it's data is fetched from pokeapi
            if (!alreadyExists) {
                string description = fetchItemDescription(itemName);

                //verifies web database data before saving it
                if (!description.empty()) {
                    PokemonItem newItem = {itemName, description, qty};
                    organizerInventory.push_back(newItem); 
                    cout << itemName << " was added to your inventory" << endl;
                } else {
                    cout << "Item not found" << endl;
                }
            }
        }
        //option 2----------------------------------------------------------------------------------------------------------
        else if (choice == 2) {
            //checks if array is empty
            if (organizerInventory.empty()) {
                cout << "Inventory is empty" << endl;
            } else {
                cout << "\nInventory------------------------" << endl;
                //iterates through vector to display items and descriptions
                for (size_t i = 0; i < organizerInventory.size(); i++) {
                    cout << "[" << i + 1 << "] " << organizerInventory[i].name 
                         << " | Quantity: " << organizerInventory[i].quantity << endl;
                    cout << "    Effect: " << organizerInventory[i].effect << endl;
                }
            }
        }
        //option 3----------------------------------------------------------------------------------------------------------
        else if (choice == 3) {
            string rawItemName;
            int newQty;
            cout << "Enter item to update: ";
            cin >> rawItemName;
            cout << "Enter new quantity: ";
            cin >> newQty;
            cin.ignore(1000, '\n');

            string itemName = cleanString(rawItemName);
            bool updated = false;
            //loops through structures to find and overwrite the requested quantity
            for (size_t i = 0; i < organizerInventory.size(); i++) {
                if (organizerInventory[i].name == itemName) {
                    organizerInventory[i].quantity = newQty; 
                    cout << "Quantity overwritten" << endl;
                    updated = true;
                    break;
                }
            }
            if (!updated) cout << "Item not found in inventory" << endl;
        }
        //option 4----------------------------------------------------------------------------------------------------------
        else if (choice == 4) {
            string rawItemName;
            cout << "Enter item to delete: ";
            cin >> rawItemName;
            cin.ignore(1000, '\n');

            string itemName = cleanString(rawItemName);
            bool removed = false;
            //search through vector to delete requested item
            for (auto it = organizerInventory.begin(); it != organizerInventory.end(); it++) {
                if (it->name == itemName) {
                    it = organizerInventory.erase(it); 
                    cout << itemName << " was deleted" << endl;
                    removed = true;
                    break;
                }
            }
            if (!removed) cout << "Item to delete not found" << endl;
        }
        else {
            cout << "Invalid input; type a number 1-5" << endl;
        }
    }
    return 0;
}
