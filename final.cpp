#include <iostream>
#include <vector>
#include <string>
#include <Windows.h>
#include <WinInet.h>

#pragma comment(lib, "wininet.lib")

using namespace std;

//this structure used by the vector groups an item's data together into a single entry
struct PokemonItem {
    string name;
    string category;
    int cost;
    int flingPower;
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

//extracts a flat string value from raw json by searching for "key":"value"
string extractStringField(const string& body, const string& key) {
    string search = "\"" + key + "\":\"";
    size_t pos = body.find(search);
    if (pos == string::npos) return "";
    pos += search.size();
    size_t end = body.find("\"", pos);
    if (end == string::npos) return "";
    return body.substr(pos, end - pos);
}

//extracts a flat integer value from raw json by searching for "key":number
int extractIntField(const string& body, const string& key) {
    string search = "\"" + key + "\":";
    size_t pos = body.find(search);
    if (pos == string::npos) return -1;
    pos += search.size();
    if (body[pos] == 'n') return -1; //handles null
    size_t end = body.find_first_not_of("0123456789", pos);
    return stoi(body.substr(pos, end - pos));
}

//extracts a nested name field by finding the parent key, then the name inside it
string extractNestedName(const string& body, const string& parentKey) {
    string search = "\"" + parentKey + "\":{";
    size_t pos = body.find(search);
    if (pos == string::npos) return "";
    size_t blockEnd = body.find("}", pos);
    if (blockEnd == string::npos) return "";
    string block = body.substr(pos, blockEnd - pos);
    return extractStringField(block, "name");
}

string fetchItemData(const string& itemName, string& outCategory, int& outCost, int& outFlingPower) {
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
    string path = "/api/v2/item/" + itemName + "/";
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

    if (resultBody.empty() || resultBody.find("Not Found") != string::npos) return "";

    //parse each field from the raw response
    outCost = extractIntField(resultBody, "cost");
    outFlingPower = extractIntField(resultBody, "fling_power");
    outCategory = extractNestedName(resultBody, "category");

    return resultBody;
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

        //catch and fix typing errors
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(1000, '\n');
            continue;
        }
        cin.ignore(1000, '\n'); //clears leftover newline memory (important because this would mess with other inputs)

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
                string category;
                int cost = -1, flingPower = -1;
                string result = fetchItemData(itemName, category, cost, flingPower);

                //verifies database data before saving item
                if (!result.empty()) {
                    PokemonItem newItem = {
                        itemName,
                        category.empty() ? "unknown" : category,
                        cost,
                        flingPower,
                        qty
                    };
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
                         << " | Quantity: " << organizerInventory[i].quantity
                         << " | Category: " << organizerInventory[i].category
                         << " | Cost (Pokedollars): " << (organizerInventory[i].cost >= 0 ? to_string(organizerInventory[i].cost) : "N/A")
                         << " | Fling Power: " << (organizerInventory[i].flingPower >= 0 ? to_string(organizerInventory[i].flingPower) : "N/A")
                         << endl;
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
                    cout << "Quantity updated" << endl;
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
