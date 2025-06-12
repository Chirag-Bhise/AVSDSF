/*
ONCO
*/
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <limits>
#include <chrono>
#include <iomanip>

// Constants and parameters
const double BASE_WEIGHT_C = 0.3; // Base weight for computation cost
const double BASE_WEIGHT_R = 0.3;  // Base weight for retention cost
const double BASE_WEIGHT_TR = 0.3; // Base weight for transfer cost
const double BASE_WEIGHT_P = 0.3;  // Base weight for preparation cost
const double RETENTION_THRESHOLD = 0.5; // Fixed threshold for container retention

struct RSU {
    int id;
    double maxCapacity;
    double usedCapacity;
    double retentionCost;
    double computationCost;
    double preparationCost;
};

struct ServiceRequest {
    int id;
    double deadline;
    double computationLoad;
    double transferCost;
    double preparationCost;
    double distanceToRSU;
};

struct DecisionVariables {
    std::unordered_map<int, int> X; // Request scheduling
    std::unordered_map<int, int> A; // Container retention
};

double previousLoad = 0.0;
std::vector<double> previousWeights = {0.5, 0.2, 0.2, 0.1};  // Initial weights

std::vector<double> calculateDynamicWeights(double load) {
    std::vector<double> weights(4);

    // Calculate the slope of the previous load change
    double loadDifference = load - previousLoad;
    double slope = 0.0;
    if (previousLoad != 0.0) {
        slope = loadDifference / previousLoad;  // Simple slope calculation (delta load / previous load)
    }

    // Adjust weights based on slope (i.e., adapt the weights)
    if (load <= 0.4) {
        weights = {0.5, 0.2, 0.2, 0.1};  // Low load
    } else if (load <= 0.7) {
        // Medium load, adjust with slope (modify weights adaptively)
        weights = {0.4 + slope * 0.1, 0.3 + slope * 0.05, 0.2 - slope * 0.05, 0.1 - slope * 0.05};
    } else {
        // High load, adjust with slope
        weights = {0.3 + slope * 0.1, 0.4 + slope * 0.1, 0.2 - slope * 0.05, 0.1 - slope * 0.05};
    }

    // Normalize the weights to ensure they sum to 1 (important for PLF)
    double sum = weights[0] + weights[1] + weights[2] + weights[3];
    for (auto& w : weights) {
        w /= sum;
    }

    // Update previous values for the next iteration
    previousLoad = load;
    previousWeights = weights;

    return weights;
}

// Compute total cost for a request using dynamic weights
double computeCost(const ServiceRequest& request, const RSU& rsu, const std::vector<double>& weights) {
    return weights[0] * rsu.computationCost * request.computationLoad +
           weights[1] * rsu.retentionCost +
           weights[2] * request.transferCost +
           weights[3] * request.preparationCost;
}

// Schedule requests to minimize cost with dynamic weights
void scheduleRequests(std::vector<ServiceRequest>& requests, std::vector<RSU>& rsus, std::vector<double>& weights, DecisionVariables& decisions, int timeSlot, std::chrono::time_point<std::chrono::high_resolution_clock>& slotStartTime) {
    auto start = std::chrono::high_resolution_clock::now();
    for (auto& request : requests) {
        double minCost = std::numeric_limits<double>::max();
        int bestRSU = -1;

        for (auto& rsu : rsus) {
            if (rsu.usedCapacity + request.computationLoad <= rsu.maxCapacity) {
                double cost = computeCost(request, rsu, weights);
                if (cost < minCost) {
                    minCost = cost;
                    bestRSU = rsu.id;
                }
            }
        }

        if (bestRSU != -1) {
            decisions.X[request.id] = bestRSU;
            rsus[bestRSU].usedCapacity += request.computationLoad;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    double schedulingLatency = std::chrono::duration<double, std::micro>(end - start).count();  // Measure latency in microseconds
    // std::cout << "Time Slot " << timeSlot << " - Request Scheduling Latency: " << schedulingLatency << " microseconds" << std::endl;
}

// Retain containers based on dynamic weights and system conditions
void retainContainers(std::vector<RSU>& rsus, DecisionVariables& decisions, double load, int timeSlot, std::chrono::time_point<std::chrono::high_resolution_clock>& slotStartTime) {
    auto start = std::chrono::high_resolution_clock::now();
    for (auto& rsu : rsus) {
        if (load <= 0.7 && rsu.retentionCost <= RETENTION_THRESHOLD) {
            decisions.A[rsu.id] = 1; // Retain container for moderate load
        } else {
            decisions.A[rsu.id] = 0; // Do not retain for high load or high cost
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    double retentionLatency = std::chrono::duration<double, std::micro>(end - start).count();  // Measure latency in microseconds
    // std::cout << "Time Slot " << timeSlot << " - Container Retention Latency: " << retentionLatency << " microseconds" << std::endl;
}

// Compute total cost
double computeTotalCost(const std::vector<ServiceRequest>& requests, const std::vector<RSU>& rsus, const DecisionVariables& decisions) {
    double totalCost = 0.0;

    for (const auto& request : requests) {
        int assignedRSU = decisions.X.at(request.id);
        const auto& rsu = rsus[assignedRSU];

        totalCost += BASE_WEIGHT_C * rsu.computationCost * request.computationLoad +
                     BASE_WEIGHT_R * rsu.retentionCost +
                     BASE_WEIGHT_TR * request.transferCost +
                     BASE_WEIGHT_P * request.preparationCost;
    }

    return totalCost;
}

// Main algorithm loop with dynamic slope-based PLF optimization
void runBaseAlgorithm(int T, std::vector<ServiceRequest>& requests, std::vector<RSU>& rsus) {
    DecisionVariables decisions;

    for (int t = 0; t < T; ++t) {
        // Compute system load
        double totalCapacity = std::accumulate(rsus.begin(), rsus.end(), 0.0, [](double sum, const RSU& rsu) {
            return sum + rsu.maxCapacity;
        });
        double usedCapacity = std::accumulate(rsus.begin(), rsus.end(), 0.0, [](double sum, const RSU& rsu) {
            return sum + rsu.usedCapacity;
        });
        double load = usedCapacity / totalCapacity;

        // Calculate dynamic weights based on load
        std::vector<double> weights = calculateDynamicWeights(load);

        // Start time for this slot
        auto slotStartTime = std::chrono::high_resolution_clock::now();

        // Schedule requests
        scheduleRequests(requests, rsus, weights, decisions, t, slotStartTime);

        // Retain containers
        retainContainers(rsus, decisions, load, t, slotStartTime);

        // Measure the overall latency
        auto slotEndTime = std::chrono::high_resolution_clock::now();
        double overallLatency = std::chrono::duration<double, std::micro>(slotEndTime - slotStartTime).count();  // Overall latency in microseconds

        // Compute total cost
        double totalCost = computeTotalCost(requests, rsus, decisions);

        // Output iteration results
        std::cout << "Time Slot " << t << ": Total Cost = " << totalCost 
                  << ", Overall Latency = " << overallLatency << " microseconds" << std::endl;
    }
}

int main() {
    // Example setup
    std::vector<RSU> rsus = {
        {0, 110.0, 0.0, 0.02, 0.03, 0.01},
        {1, 120.0, 0.0, 0.04, 0.02, 0.025},
        {2, 130.0, 0.0, 0.025, 0.05, 0.02}
    };

    std::vector<ServiceRequest> requests = {
        {0, 4.0, 25.0, 0.025, 0.02, 110.0},
        {1, 5.0, 35.0, 0.035, 0.02, 130.0},
        {2, 2.0, 12.0, 0.015, 0.008, 90.0}
    };

    int T = 5; // Number of time slots

    runBaseAlgorithm(T, requests, rsus);

    return 0;
}
