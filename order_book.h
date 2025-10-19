#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <list>
#include <memory>

// Order structure with all required fields
struct Order {
    uint64_t order_id;     // Unique order identifier
    bool is_buy;           // true = buy, false = sell
    double price;          // Limit price
    uint64_t quantity;     // Remaining quantity
    uint64_t timestamp_ns; // Order entry timestamp in nanoseconds
};

// PriceLevel represents a price level with aggregated volume
struct PriceLevel {
    double price;
    uint64_t total_quantity;
};

// Forward declaration for internal use
struct OrderNode {
    Order order;
    std::list<OrderNode>::iterator list_iter;
};

class OrderBook {
public:
    OrderBook() = default;
    ~OrderBook() = default;

    // Insert a new order into the book
    void add_order(const Order& order);

    // Cancel an existing order by its ID
    bool cancel_order(uint64_t order_id);

    // Amend an existing order's price or quantity
    bool amend_order(uint64_t order_id, double new_price, uint64_t new_quantity);

    // Get a snapshot of top N bid and ask levels (aggregated quantities)
    void get_snapshot(size_t depth, std::vector<PriceLevel>& bids, std::vector<PriceLevel>& asks) const;

    // Print current state of the order book
    void print_book(size_t depth = 10) const;

private:
    // Internal structure to maintain orders at each price level
    struct PriceLevelData {
        double price;
        std::list<OrderNode> orders; // FIFO queue of orders at this price
        uint64_t total_quantity = 0;
    };

    // Bids: highest price first (descending order)
    std::map<double, PriceLevelData, std::greater<double>> bids_;
    
    // Asks: lowest price first (ascending order)
    std::map<double, PriceLevelData, std::less<double>> asks_;
    
    // Order lookup for O(1) access
    struct OrderLocation {
        bool is_buy;
        double price;
        std::list<OrderNode>::iterator list_iter;
    };
    
    std::unordered_map<uint64_t, OrderLocation> order_lookup_;
};
