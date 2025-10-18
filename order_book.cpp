#include "order_book.h"
#include <iostream>
#include <iomanip>

void OrderBook::add_order(const Order& order) {
    // Get or create the price level
    PriceLevelData* price_level = nullptr;
    
    if (order.is_buy) {
        auto& level = bids_[order.price];
        level.price = order.price;
        price_level = &level;
    } else {
        auto& level = asks_[order.price];
        level.price = order.price;
        price_level = &level;
    }
    
    // Add the order to the FIFO queue at this price level
    OrderNode node;
    node.order = order;
    price_level->orders.push_back(node);
    
    // Get iterator to the newly added order
    auto list_iter = std::prev(price_level->orders.end());
    list_iter->list_iter = list_iter; // Self-reference for quick access
    
    // Update total quantity
    price_level->total_quantity += order.quantity;
    
    // Add to lookup table
    OrderLocation location;
    location.is_buy = order.is_buy;
    location.price = order.price;
    location.list_iter = list_iter;
    order_lookup_[order.order_id] = location;
}

bool OrderBook::cancel_order(uint64_t order_id) {
    // Look up the order
    auto lookup_it = order_lookup_.find(order_id);
    if (lookup_it == order_lookup_.end()) {
        return false; // Order not found
    }
    
    const OrderLocation& location = lookup_it->second;
    
    // Get the order from the list
    const Order& order = location.list_iter->order;
    
    if (location.is_buy) {
        // Handle bids
        auto price_it = bids_.find(location.price);
        if (price_it == bids_.end()) {
            return false; // Should not happen
        }
        
        auto& price_level = price_it->second;
        
        // Update total quantity
        price_level.total_quantity -= order.quantity;
        
        // Remove the order from the list
        price_level.orders.erase(location.list_iter);
        
        // If the price level is now empty, remove it
        if (price_level.orders.empty()) {
            bids_.erase(price_it);
        }
    } else {
        // Handle asks
        auto price_it = asks_.find(location.price);
        if (price_it == asks_.end()) {
            return false; // Should not happen
        }
        
        auto& price_level = price_it->second;
        
        // Update total quantity
        price_level.total_quantity -= order.quantity;
        
        // Remove the order from the list
        price_level.orders.erase(location.list_iter);
        
        // If the price level is now empty, remove it
        if (price_level.orders.empty()) {
            asks_.erase(price_it);
        }
    }
    
    // Remove from lookup table
    order_lookup_.erase(lookup_it);
    
    return true;
}

bool OrderBook::amend_order(uint64_t order_id, double new_price, uint64_t new_quantity) {
    // Look up the order
    auto lookup_it = order_lookup_.find(order_id);
    if (lookup_it == order_lookup_.end()) {
        return false; // Order not found
    }
    
    const OrderLocation& location = lookup_it->second;
    Order& order = location.list_iter->order;
    
    // Check if price is changing
    if (order.price != new_price) {
        // Price change: treat as cancel + add
        Order new_order = order;
        new_order.price = new_price;
        new_order.quantity = new_quantity;
        
        // Cancel the old order
        cancel_order(order_id);
        
        // Add the new order
        add_order(new_order);
    } else {
        // Only quantity is changing: update in place
        if (location.is_buy) {
            auto price_it = bids_.find(location.price);
            if (price_it == bids_.end()) {
                return false; // Should not happen
            }
            
            auto& price_level = price_it->second;
            
            // Update total quantity at price level
            price_level.total_quantity = price_level.total_quantity - order.quantity + new_quantity;
            
            // Update the order
            order.quantity = new_quantity;
        } else {
            auto price_it = asks_.find(location.price);
            if (price_it == asks_.end()) {
                return false; // Should not happen
            }
            
            auto& price_level = price_it->second;
            
            // Update total quantity at price level
            price_level.total_quantity = price_level.total_quantity - order.quantity + new_quantity;
            
            // Update the order
            order.quantity = new_quantity;
        }
    }
    
    return true;
}

void OrderBook::get_snapshot(size_t depth, std::vector<PriceLevel>& bids, std::vector<PriceLevel>& asks) const {
    bids.clear();
    asks.clear();
    
    // Collect top N bids (highest prices first)
    size_t count = 0;
    for (const auto& [price, price_level] : bids_) {
        if (count >= depth) break;
        
        PriceLevel level;
        level.price = price;
        level.total_quantity = price_level.total_quantity;
        bids.push_back(level);
        
        count++;
    }
    
    // Collect top N asks (lowest prices first)
    count = 0;
    for (const auto& [price, price_level] : asks_) {
        if (count >= depth) break;
        
        PriceLevel level;
        level.price = price;
        level.total_quantity = price_level.total_quantity;
        asks.push_back(level);
        
        count++;
    }
}

void OrderBook::print_book(size_t depth) const {
    std::vector<PriceLevel> bids, asks;
    get_snapshot(depth, bids, asks);
    
    std::cout << "\n========== ORDER BOOK ==========" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    
    // Print asks in reverse order (highest to lowest)
    std::cout << "\n--- ASKS (Sell Orders) ---" << std::endl;
    for (auto it = asks.rbegin(); it != asks.rend(); ++it) {
        std::cout << "  " << std::setw(10) << it->total_quantity 
                  << " @ $" << std::setw(8) << it->price << std::endl;
    }
    
    std::cout << "\n--------------------------" << std::endl;
    
    // Print bids (highest to lowest)
    std::cout << "--- BIDS (Buy Orders) ---" << std::endl;
    for (const auto& bid : bids) {
        std::cout << "  " << std::setw(10) << bid.total_quantity 
                  << " @ $" << std::setw(8) << bid.price << std::endl;
    }
    
    std::cout << "==============================\n" << std::endl;
}
