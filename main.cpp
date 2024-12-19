#include <iostream>
#include <vector>
#include "ScanOperator.h"
#include "FilterOperator.h"
#include "Consumer.h"

int main() {
    // Sample data
    std::vector<Row> data = {
        {1, 10},
        {2, 20},
        {3, 30}
    };

    // Predicate function for filtering
    auto predicate = [](const Row& row) {
        return row.value > 15;
    };

    // Create operators
    ScanOperator scan(data);
    FilterOperator filter(predicate);
    Consumer consumer;

    // Set up the pipeline
    scan.setConsumer(&filter);
    filter.setConsumer(&consumer);

    // Execute the query
    scan.open();
    scan.next();
    scan.close();

    // Get the results
    const auto& results = consumer.getResults();
    for (const auto& row : results) {
        std::cout << "Row: { id: " << row.id << ", value: " << row.value << " }\n";
    }

    return 0;
}