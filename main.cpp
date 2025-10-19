#include "order_book.h"
#include <iostream>
#include <chrono>
#include <cassert>

// Helper function to get current timestamp in nanoseconds
uint64_t get_timestamp_ns() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

// Test basic add order functionality
void test_add_orders() {
    std::cout << "\n=== Test: Add Orders ===" << std::endl;
    OrderBook book;
    
    // Add some buy orders
    Order order1 = {1, true, 100.0, 50, get_timestamp_ns()};
    Order order2 = {2, true, 100.0, 30, get_timestamp_ns()};
    Order order3 = {3, true, 99.0, 100, get_timestamp_ns()};
    
    book.add_order(order1);
    book.add_order(order2);
    book.add_order(order3);
    
    // Add some sell orders
    Order order4 = {4, false, 101.0, 40, get_timestamp_ns()};
    Order order5 = {5, false, 102.0, 60, get_timestamp_ns()};
    Order order6 = {6, false, 101.0, 20, get_timestamp_ns()};
    
    book.add_order(order4);
    book.add_order(order5);
    book.add_order(order6);
    
    book.print_book();
    
    // Verify snapshot
    std::vector<PriceLevel> bids, asks;
    book.get_snapshot(5, bids, asks);
    
    assert(bids.size() == 2);
    assert(bids[0].price == 100.0);
    assert(bids[0].total_quantity == 80); // 50 + 30
    assert(bids[1].price == 99.0);
    assert(bids[1].total_quantity == 100);
    
    assert(asks.size() == 2);
    assert(asks[0].price == 101.0);
    assert(asks[0].total_quantity == 60); // 40 + 20
    assert(asks[1].price == 102.0);
    assert(asks[1].total_quantity == 60);
    
    std::cout << "✓ Add orders test passed" << std::endl;
}

// Test cancel order functionality
void test_cancel_order() {
    std::cout << "\n=== Test: Cancel Order ===" << std::endl;
    OrderBook book;
    
    Order order1 = {1, true, 100.0, 50, get_timestamp_ns()};
    Order order2 = {2, true, 100.0, 30, get_timestamp_ns()};
    Order order3 = {3, true, 99.0, 100, get_timestamp_ns()};
    
    book.add_order(order1);
    book.add_order(order2);
    book.add_order(order3);
    
    std::cout << "Before cancel:" << std::endl;
    book.print_book();
    
    // Cancel order 2
    bool result = book.cancel_order(2);
    assert(result == true);
    
    std::cout << "After canceling order 2:" << std::endl;
    book.print_book();
    
    // Verify snapshot
    std::vector<PriceLevel> bids, asks;
    book.get_snapshot(5, bids, asks);
    
    assert(bids[0].price == 100.0);
    assert(bids[0].total_quantity == 50); // Only order 1 remains
    
    // Try to cancel non-existent order
    result = book.cancel_order(999);
    assert(result == false);
    
    std::cout << "✓ Cancel order test passed" << std::endl;
}

// Test amend order functionality
void test_amend_order() {
    std::cout << "\n=== Test: Amend Order ===" << std::endl;
    OrderBook book;
    
    Order order1 = {1, true, 100.0, 50, get_timestamp_ns()};
    Order order2 = {2, false, 101.0, 40, get_timestamp_ns()};
    
    book.add_order(order1);
    book.add_order(order2);
    
    std::cout << "Before amend:" << std::endl;
    book.print_book();
    
    // Amend quantity only
    bool result = book.amend_order(1, 100.0, 75);
    assert(result == true);
    
    std::cout << "After amending quantity (order 1, 50->75):" << std::endl;
    book.print_book();
    
    std::vector<PriceLevel> bids, asks;
    book.get_snapshot(5, bids, asks);
    assert(bids[0].total_quantity == 75);
    
    // Amend price (should be treated as cancel + add)
    result = book.amend_order(1, 99.5, 75);
    assert(result == true);
    
    std::cout << "After amending price (order 1, 100.0->99.5):" << std::endl;
    book.print_book();
    
    book.get_snapshot(5, bids, asks);
    assert(bids[0].price == 99.5);
    assert(bids[0].total_quantity == 75);
    
    // Try to amend non-existent order
    result = book.amend_order(999, 100.0, 10);
    assert(result == false);
    
    std::cout << "✓ Amend order test passed" << std::endl;
}

// Test snapshot with different depths
void test_snapshot_depth() {
    std::cout << "\n=== Test: Snapshot Depth ===" << std::endl;
    OrderBook book;
    
    // Add multiple price levels
    for (int i = 0; i < 10; i++) {
        Order buy_order = {static_cast<uint64_t>(i), true, 100.0 - i, 100, get_timestamp_ns()};
        Order sell_order = {static_cast<uint64_t>(100 + i), false, 101.0 + i, 100, get_timestamp_ns()};
        book.add_order(buy_order);
        book.add_order(sell_order);
    }
    
    // Test different depths
    std::vector<PriceLevel> bids, asks;
    
    book.get_snapshot(3, bids, asks);
    assert(bids.size() == 3);
    assert(asks.size() == 3);
    assert(bids[0].price == 100.0); // Highest bid
    assert(asks[0].price == 101.0); // Lowest ask
    
    book.get_snapshot(15, bids, asks);
    assert(bids.size() == 10); // Max available
    assert(asks.size() == 10); // Max available
    
    std::cout << "Top 5 levels:" << std::endl;
    book.print_book(5);
    
    std::cout << "✓ Snapshot depth test passed" << std::endl;
}

// Test FIFO priority within price level
void test_fifo_priority() {
    std::cout << "\n=== Test: FIFO Priority ===" << std::endl;
    OrderBook book;
    
    // Add multiple orders at the same price
    Order order1 = {1, true, 100.0, 50, get_timestamp_ns()};
    Order order2 = {2, true, 100.0, 30, get_timestamp_ns()};
    Order order3 = {3, true, 100.0, 20, get_timestamp_ns()};
    
    book.add_order(order1);
    book.add_order(order2);
    book.add_order(order3);
    
    // Total should be 100
    std::vector<PriceLevel> bids, asks;
    book.get_snapshot(5, bids, asks);
    assert(bids[0].total_quantity == 100);
    
    // Cancel the first order
    book.cancel_order(1);
    book.get_snapshot(5, bids, asks);
    assert(bids[0].total_quantity == 50); // 30 + 20
    
    // Cancel the middle order
    book.cancel_order(2);
    book.get_snapshot(5, bids, asks);
    assert(bids[0].total_quantity == 20); // Only order3 remains
    
    std::cout << "✓ FIFO priority test passed" << std::endl;
}

// Performance test
void test_performance() {
    std::cout << "\n=== Test: Performance ===" << std::endl;
    OrderBook book;
    
    const int num_orders = 10000;
    
    // Test add performance
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_orders; i++) {
        Order order = {
            static_cast<uint64_t>(i),
            i % 2 == 0,
            100.0 + (i % 100) * 0.01,
            100,
            get_timestamp_ns()
        };
        book.add_order(order);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Added " << num_orders << " orders in " << duration.count() << " µs" << std::endl;
    std::cout << "Average: " << duration.count() / num_orders << " µs per order" << std::endl;
    
    // Test cancel performance
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_orders / 2; i++) {
        book.cancel_order(i * 2);
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Canceled " << num_orders / 2 << " orders in " << duration.count() << " µs" << std::endl;
    std::cout << "Average: " << duration.count() / (num_orders / 2) << " µs per cancel" << std::endl;
    
    // Test snapshot performance
    start = std::chrono::high_resolution_clock::now();
    std::vector<PriceLevel> bids, asks;
    for (int i = 0; i < 1000; i++) {
        book.get_snapshot(10, bids, asks);
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "1000 snapshots in " << duration.count() << " µs" << std::endl;
    std::cout << "Average: " << duration.count() / 1000 << " µs per snapshot" << std::endl;
    
    std::cout << "✓ Performance test completed" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Low-Latency Limit Order Book Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        test_add_orders();
        test_cancel_order();
        test_amend_order();
        test_snapshot_depth();
        test_fifo_priority();
        test_performance();
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "  ✓ All tests passed successfully!" << std::endl;
        std::cout << "========================================\n" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
