#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>

using namespace std;

// Structure to organize our local inventory data
struct PokemonItem {
    string name;
    string effect;
    int quantity; 
};

// Function helper to handle the live PokeAPI curl call
string fetchItemDescription(const string& itemName) {
    string tempFilePath = "/tmp/poke_response.txt";
    
    // MAC POWER COMMAND: Uses curl to fetch, and lets Mac's native 'grep' extract the short_effect instantly
    string command = "curl -k -s -L -A 'Mozilla/5.0' \"https://pokeapi.co" + itemName + "\" | grep -o '\"short_effect\":\"[^\"]*\"' > " + tempFilePath;
    
    int sysResult = system(command.c_str());

    ifstream file(tempFilePath);
    string rawLine;
    if (file && sysResult == 0) {
        getline(file, rawLine);
        file.close();
        remove(tempFilePath.c_str());
    }

    // If grep found the description match, clean it up
    // Example rawLine: "short_effect":"Restores 20 HP."
    if (!rawLine.empty()) {
        size_t pos = rawLine.find("\":\"");
        if (pos != string::npos) {
            string cleanEffect = rawLine.substr(pos + 3); // Cut off everything before the description text
            if (!cleanEffect.empty() && cleanEffect.back() == '"') {
                cleanEffect.pop_back(); // Remove the trailing quote
            }
            return cleanEffect;
        }
    }
    return ""; 
}

int main() {
    vector<PokemonItem> organizerInventory;
    int choice = 0;

    cout << "========================================" << endl;
    cout << "   LOCAL POKEMON ITEM ORGANIZER MENU   " << endl;
    cout << "========================================" << endl;

    while (true) {
        cout << "\n1. Add/Stack Item\n2. View Local Inventory\n3. Update Quantity\n4. Remove Item\n5. Exit Program\n";
        cout << "Enter your choice (1-5): ";
        cin >> choice;

        // CRITICAL FIX: Clears the leftover newline memory from typing a number
        cin.ignore(1000, '\n'); 

        if (choice == 5) {
            cout << "Exiting program. Goodbye!" << endl;
            break;
        }

        // --- OPTION 1: ADD AN ITEM ---
        if (choice == 1) {
            string itemName;
            int qty;
            cout << "Enter item name (e.g., master-ball, potion): ";
            cin >> itemName;
            cout << "Enter quantity: ";
            cin >> qty;
            cin.ignore(1000, '\n'); // Clear stream again after inputs

            bool alreadyExists = false;
            for (size_t i = 0; i < organizerInventory.size(); ++i) {
                if (organizerInventory[i].name == itemName) {
                    organizerInventory[i].quantity += qty;
                    cout << "--> Updated existing stack! Total is now: " << organizerInventory[i].quantity << endl;
                    alreadyExists = true;
                    break;
                }
            }

            if (!alreadyExists) {
                cout << "Connecting to PokeAPI to look up item specs..." << endl;
                string description = fetchItemDescription(itemName);

                if (!description.empty()) {
                    PokemonItem newItem = {itemName, description, qty};
                    organizerInventory.push_back(newItem);
                    cout << "--> Successfully added " << itemName << " to your vector inventory!" << endl;
                } else {
                    cout << "--> Error: Real PokeAPI could not find that item. Check your spelling (use lowercase and hyphens like ultra-ball)." << endl;
                }
            }
        }
        // --- OPTION 2: VIEW LOCAL INVENTORY ---
        else if (choice == 2) {
            if (organizerInventory.empty()) {
                cout << "--> Your inventory vector is completely empty." << endl;
            } else {
                cout << "\n--- CURRENT INVENTORY CONSOLE REPORT ---" << endl;
                for (size_t i = 0; i < organizerInventory.size(); ++i) {
                    cout << "[" << i + 1 << "] " << organizerInventory[i].name 
                         << " | Qty: " << organizerInventory[i].quantity << endl;
                    cout << "    Effect: " << organizerInventory[i].effect << endl;
                }
                cout << "----------------------------------------" << endl;
            }
        }
        // --- OPTION 3: UPDATE QUANTITY ---
        else if (choice == 3) {
            string itemName;
            int newQty;
            cout << "Enter item name to update: ";
            cin >> itemName;
            cout << "Enter exact new quantity: ";
            cin >> newQty;
            cin.ignore(1000, '\n');

            bool updated = false;
            for (size_t i = 0; i < organizerInventory.size(); ++i) {
                if (organizerInventory[i].name == itemName) {
                    organizerInventory[i].quantity = newQty;
                    cout << "--> Quantity overwritten successfully!" << endl;
                    updated = true;
                    break;
                }
            }
            if (!updated) cout << "--> Item not found in your inventory." << endl;
        }
        // --- OPTION 4: REMOVE ITEM ---
        else if (choice == 4) {
            string itemName;
            cout << "Enter item name to remove entirely: ";
            cin >> itemName;
            cin.ignore(1000, '\n');

            bool removed = false;
            for (auto it = organizerInventory.begin(); it != organizerInventory.end(); ++it) {
                if (it->name == itemName) {
                    organizerInventory.erase(it);
                    cout << "--> " << itemName << " completely dropped from vector." << endl;
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