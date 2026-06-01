#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <Windows.h>
#include <WinInet.h>

//built-in windows internet library (lets us do web requests)
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
    string cleaned = "";
    for (char c : str) {
        if (c != '\r' && c != '\n' && c != ' ') {
            cleaned += tolower(c); 
        }
    }
    return cleaned;
}

//connects to pokeapi to get descriptions
string fetchItemDescription(const string& itemName) {
    string resultBody = "";
    
    //init native windows internet sessions
    HINTERNET hInternet = InternetOpenA("PokeAgent", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return "";

    //connect to pokeapi server layout
    HINTERNET hConnect = InternetConnectA(hInternet, "pokeapi.co", INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return "";
    }

    //opens request channel
    string path = "/api/v2/item/" + itemName;
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

    //if valid, parse description text
    if (!resultBody.empty() && resultBody.find("Not Found") == string::npos) {
        size_t targetPos = resultBody.find("\"short_effect\":\"");
        if (targetPos != string::npos) {
            size_t start = targetPos + 16;
            size_t end = resultBody.find("\"", start);
            if (end != string::npos) {
                return resultBody.substr(start, end - start);
            }
        }
    }
    return ""; 
}

int main() {
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
            cout << "Exiting program. Goodbye!" << endl;
            break;
        }

        //option 1----------------------------------------------------------------------------------------------------------
        if (choice == 1) {
            string rawItemName;
            int qty;
            cout << "Enter item name (e.g., master-ball, potion): ";
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
                    cout << "--> Updated existing stack! Total is now: " << organizerInventory[i].quantity << endl;
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
                         << " | Qty: " << organizerInventory[i].quantity << endl;
                    cout << "    Effect: " << organizerInventory[i].effect << endl;
                }
            }
        }
        //option 3----------------------------------------------------------------------------------------------------------
        else if (choice == 3) {
            string rawItemName;
            int newQty;
            cout << "Enter item name to update: ";
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
                    cout << "--> Quantity overwritten successfully!" << endl;
                    updated = true;
                    break;
                }
            }
            if (!updated) cout << "--> Item not found in your inventory." << endl;
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
                    cout << "--> " << itemName << " completely dropped from vector tracker." << endl;
                    removed = true;
                    break;
                }
            }
            if (!removed) cout << "--> Could not find that item to delete." << endl;
        }
        else {
            cout << "Invalid entry. Type a number 1 through 5." << endl;
        }
    }

    return 0;
}
