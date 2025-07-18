#include <iostream>
#include <string>
#include <unordered_map>
#include <queue>
#include <vector>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <sstream>

using namespace std;
using namespace chrono;

// -------------------- Bid --------------------
struct Bid {
    string userId;
    double amount;
    time_point<steady_clock> timestamp;
    string itemId;

    Bid(const string& uid, double amt, const string& iid)
        : userId(uid), amount(amt), timestamp(steady_clock::now()), itemId(iid) {}

    Bid() : userId(""), amount(0.0), itemId(""), timestamp(steady_clock::now()) {}

    bool operator<(const Bid& other) const {
        if (amount != other.amount)
            return amount < other.amount; // Max heap
        return timestamp > other.timestamp; // Earlier wins on tie
    }
};

// -------------------- Item --------------------
struct Item {
    string id;
    string name;
    string description;
    double startingPrice;
    double reservePrice;
    string sellerId;
    time_point<steady_clock> startTime;
    time_point<steady_clock> endTime;
    bool isActive;

    // Default constructor
    Item() : id(""), name(""), description(""), startingPrice(0.0), reservePrice(0.0),
             sellerId(""), isActive(false), startTime(steady_clock::now()), endTime(steady_clock::now()) {}

    // Parameterized constructor
    Item(const string& itemId, const string& itemName, const string& desc,
         double startPrice, double reserve, const string& seller, int durationMinutes)
        : id(itemId), name(itemName), description(desc), startingPrice(startPrice),
          reservePrice(reserve), sellerId(seller), isActive(true) {
        startTime = steady_clock::now();
        endTime = startTime + minutes(durationMinutes);
    }

    bool isExpired() const {
        return steady_clock::now() > endTime;
    }

    int getRemainingSeconds() const {
        auto remaining = duration_cast<seconds>(endTime - steady_clock::now());
        return max(0, (int)remaining.count());
    }
};

// -------------------- User --------------------
class User {
public:
    string id;
    string username;
    string email;
    double balance;
    vector<string> bidHistory;
    vector<string> ownedItems;
    vector<string> soldItems;

    User() : id(""), username(""), email(""), balance(0.0) {}

    User(const string& userId, const string& uname, const string& mail, double bal = 0.0)
        : id(userId), username(uname), email(mail), balance(bal) {}

    bool canBid(double amount) const {
        return balance >= amount;
    }

    void deductBalance(double amount) {
        if (balance >= amount)
            balance -= amount;
    }

    void addBalance(double amount) {
        balance += amount;
    }

    void addBidToHistory(const string& itemId) {
        bidHistory.push_back(itemId);
    }

    void addOwnedItem(const string& itemId) {
        ownedItems.push_back(itemId);
    }

    void addSoldItem(const string& itemId) {
        soldItems.push_back(itemId);
    }
};

// -------------------- Auction --------------------
class Auction {
private:
    Item item;
    priority_queue<Bid> bids;
    unordered_map<string, double> userHighestBids;
    vector<Bid> bidHistory;

public:
    Auction() {}
    Auction(const Item& itm) : item(itm) {}

    bool isActive() const {
        return item.isActive && !item.isExpired();
    }

    void endAuction() {
        item.isActive = false;
    }

    bool placeBid(const string& userId, double amount) {
        if (!isActive()) {
            cout << "Auction is not active!" << endl;
            return false;
        }

        if (amount <= item.startingPrice) {
            cout << "Bid must be higher than starting price: $" << item.startingPrice << endl;
            return false;
        }

        if (!bids.empty() && amount <= bids.top().amount) {
            cout << "Bid must be higher than current highest bid: $" << bids.top().amount << endl;
            return false;
        }

        if (userId == item.sellerId) {
            cout << "Cannot bid on your own item!" << endl;
            return false;
        }

        Bid newBid(userId, amount, item.id);
        bids.push(newBid);
        bidHistory.push_back(newBid);
        userHighestBids[userId] = max(userHighestBids[userId], amount);

        cout << "Bid placed successfully! Current highest bid: $" << amount << endl;
        return true;
    }

    Bid getHighestBid() const {
        if (bids.empty()) return Bid();
        return bids.top();
    }

    double getCurrentPrice() const {
        if (bids.empty()) return item.startingPrice;
        return bids.top().amount;
    }

    const Item& getItem() const {
        return item;
    }

    vector<Bid> getBidHistory() const {
        return bidHistory;
    }

    unordered_map<string, double> getUserBids() const {
        return userHighestBids;
    }

    bool hasReserveBeenMet() const {
        return getCurrentPrice() >= item.reservePrice;
    }

    void displayAuctionInfo() const {
        cout << "\n=== Auction Details ===" << endl;
        cout << "Item: " << item.name << " (ID: " << item.id << ")" << endl;
        cout << "Description: " << item.description << endl;
        cout << "Starting Price: $" << item.startingPrice << endl;
        cout << "Reserve Price: $" << item.reservePrice << endl;
        cout << "Current Price: $" << getCurrentPrice() << endl;
        cout << "Seller: " << item.sellerId << endl;
        cout << "Status: " << (isActive() ? "Active" : "Ended") << endl;
        cout << "Time Remaining: " << item.getRemainingSeconds() << " seconds" << endl;
        cout << "Reserve Met: " << (hasReserveBeenMet() ? "Yes" : "No") << endl;
        cout << "Total Bids: " << bidHistory.size() << endl;

        if (!bids.empty()) {
            cout << "Highest Bidder: " << bids.top().userId << endl;
        }
    }
};

// -------------------- Auction System --------------------
class AuctionSystem {
private:
    unordered_map<string, User> users;
    unordered_map<string, Auction> auctions;
    unordered_map<string, vector<string>> userAuctions;
    string currentUserId;

    string generateId() {
        static int counter = 1000;
        return "ID" + to_string(counter++);
    }

public:
    bool registerUser(const string& username, const string& email, double initialBalance = 1000.0) {
        for (const auto& pair : users)
            if (pair.second.username == username) {
                cout << "Username already exists!" << endl;
                return false;
            }

        string userId = generateId();
        users[userId] = User(userId, username, email, initialBalance);
        cout << "User registered successfully! User ID: " << userId << endl;
        return true;
    }

    bool loginUser(const string& username) {
        for (const auto& pair : users)
            if (pair.second.username == username) {
                currentUserId = pair.first;
                cout << "Login successful! Welcome " << username << endl;
                return true;
            }
        cout << "User not found!" << endl;
        return false;
    }

    void logoutUser() {
        currentUserId = "";
        cout << "Logged out successfully!" << endl;
    }

    bool createAuction(const string& itemName, const string& description, double startingPrice, double reservePrice, int durationMinutes) {
        if (currentUserId.empty()) {
            cout << "Please login first!" << endl;
            return false;
        }

        string itemId = generateId();
        Item item(itemId, itemName, description, startingPrice, reservePrice, currentUserId, durationMinutes);
        auctions[itemId] = Auction(item);
        userAuctions[currentUserId].push_back(itemId);

        cout << "Auction created successfully! Item ID: " << itemId << endl;
        return true;
    }

    bool placeBid(const string& itemId, double amount) {
        if (currentUserId.empty()) {
            cout << "Please login first!" << endl;
            return false;
        }

        if (auctions.find(itemId) == auctions.end()) {
            cout << "Auction not found!" << endl;
            return false;
        }

        if (!users[currentUserId].canBid(amount)) {
            cout << "Insufficient balance! Your balance: $" << users[currentUserId].balance << endl;
            return false;
        }

        if (auctions[itemId].placeBid(currentUserId, amount)) {
            users[currentUserId].addBidToHistory(itemId);
            return true;
        }
        return false;
    }

    void displayActiveAuctions() const {
        cout << "\n=== Active Auctions ===" << endl;
        bool hasActive = false;

        for (const auto& pair : auctions) {
            if (pair.second.isActive()) {
                hasActive = true;
                const auto& item = pair.second.getItem();
                cout << "ID: " << item.id << " | " << item.name
                     << " | Current Price: $" << pair.second.getCurrentPrice()
                     << " | Time Left: " << item.getRemainingSeconds() << "s" << endl;
            }
        }

        if (!hasActive)
            cout << "No active auctions available." << endl;
    }

    void displayAuctionDetails(const string& itemId) {
        if (auctions.find(itemId) == auctions.end()) {
            cout << "Auction not found!" << endl;
            return;
        }
        auctions.at(itemId).displayAuctionInfo();
    }

    void displayUserProfile() const {
        if (currentUserId.empty()) {
            cout << "Please login first!" << endl;
            return;
        }

        const User& user = users.at(currentUserId);
        cout << "\n=== User Profile ===" << endl;
        cout << "Username: " << user.username << endl;
        cout << "Email: " << user.email << endl;
        cout << "Balance: $" << user.balance << endl;
        cout << "Bids Placed: " << user.bidHistory.size() << endl;
        cout << "Items Owned: " << user.ownedItems.size() << endl;
        cout << "Items Sold: " << user.soldItems.size() << endl;

        if (userAuctions.count(currentUserId))
            cout << "Auctions Created: " << userAuctions.at(currentUserId).size() << endl;
    }

    void endAuction(const string& itemId) {
        if (auctions.find(itemId) == auctions.end()) {
            cout << "Auction not found!" << endl;
            return;
        }

        Auction& auction = auctions[itemId];
        if (!auction.isActive()) {
            cout << "Auction already ended!" << endl;
            return;
        }

        auction.endAuction();
        Bid highestBid = auction.getHighestBid();

        cout << "\n=== Auction Ended ===" << endl;
        if (highestBid.userId.empty()) {
            cout << "No bids were placed. Item remains unsold." << endl;
        } else if (!auction.hasReserveBeenMet()) {
            cout << "Reserve price not met. Item remains unsold." << endl;
        } else {
            cout << "Item sold to " << highestBid.userId << " for $" << highestBid.amount << endl;
            users[highestBid.userId].deductBalance(highestBid.amount);
            users[highestBid.userId].addOwnedItem(itemId);
            const auto& item = auction.getItem();
            users[item.sellerId].addBalance(highestBid.amount);
            users[item.sellerId].addSoldItem(itemId);
        }
    }

    void addBalance(double amount) {
        if (currentUserId.empty()) {
            cout << "Please login first!" << endl;
            return;
        }

        users[currentUserId].addBalance(amount);
        cout << "Balance added successfully! New balance: $" << users[currentUserId].balance << endl;
    }

    void displayMenu() const {
        cout << "\n=== Auction Menu ===\n";
        cout << "1. Register User\n2. Login\n3. Logout\n4. Create Auction\n5. Place Bid\n";
        cout << "6. View Active Auctions\n7. View Auction Details\n8. View User Profile\n";
        cout << "9. End Auction\n10. Add Balance\n0. Exit\nChoice: ";
    }

    void run() {
        int choice;
        string username, email, itemName, description, itemId;
        double startPrice, reservePrice, amount;
        int duration;

        cout << "Welcome to the Auction System!" << endl;

        while (true) {
            displayMenu();
            cin >> choice;
            cin.ignore();

            switch (choice) {
                case 1:
                    cout << "Username: "; getline(cin, username);
                    cout << "Email: "; getline(cin, email);
                    registerUser(username, email);
                    break;
                case 2:
                    cout << "Username: "; getline(cin, username);
                    loginUser(username);
                    break;
                case 3:
                    logoutUser(); break;
                case 4:
                    cout << "Item Name: "; getline(cin, itemName);
                    cout << "Description: "; getline(cin, description);
                    cout << "Start Price: $"; cin >> startPrice;
                    cout << "Reserve Price: $"; cin >> reservePrice;
                    cout << "Duration (minutes): "; cin >> duration;
                    cin.ignore();
                    createAuction(itemName, description, startPrice, reservePrice, duration);
                    break;
                case 5:
                    cout << "Item ID: "; getline(cin, itemId);
                    cout << "Bid Amount: $"; cin >> amount;
                    cin.ignore();
                    placeBid(itemId, amount);
                    break;
                case 6:
                    displayActiveAuctions(); break;
                case 7:
                    cout << "Item ID: "; getline(cin, itemId);
                    displayAuctionDetails(itemId);
                    break;
                case 8:
                    displayUserProfile(); break;
                case 9:
                    cout << "Item ID: "; getline(cin, itemId);
                    endAuction(itemId);
                    break;
                case 10:
                    cout << "Amount to Add: $"; cin >> amount; cin.ignore();
                    addBalance(amount); break;
                case 0:
                    cout << "Goodbye!" << endl; return;
                default:
                    cout << "Invalid choice." << endl;
            }
        }
    }
};

// -------------------- main --------------------
int main() {
    AuctionSystem system;
    system.run();
    return 0;
}
