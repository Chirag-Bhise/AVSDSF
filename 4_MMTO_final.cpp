/*
MMTO
*/
#include <iostream>
#include <vector>
#include <cmath>
#include <numeric>
#include <unordered_map>
#include <limits>
#include <random>
#include <chrono>

// Constants and parameters
const double GAMMA = 1.0; // Sensitivity for dynamic weight adjustment
const double DELTA_C = 0.3;  // Load threshold for weight adjustment
const double PREFETCH_COST_MULTIPLIER = 0.05; // Prefetching cost multiplier
const double TRANSFER_COST_MULTIPLIER = 0.1; // Transfer workload penalty

// RSU structure
struct RSU {
    int id;
    double maxCapacity;
    double usedCapacity;
    double retentionCost;
    double computationCost;
    double preparationCost;
};

// Service request structure
struct ServiceRequest {
    int id;
    double deadline;
    double computationLoad;
    double transferCost;
    double preparationCost;
    double demand;
    double distanceToRSU;
};

// Prefetched service structure
struct PrefetchedService {
    int id;
    double size; // Storage size of the service
    double prefetchCost; // Prefetching cost
};

// Decision variables
struct DecisionVariables {
    std::unordered_map<int, int> X; // Request scheduling
    std::unordered_map<int, int> A; // Container retention
    std::unordered_map<int, int> P; // Prefetching decisions
    std::unordered_map<int, int> T; // Transfer decisions
};

// Compute dynamic weights based on system load
std::vector<double> computeDynamicWeights(double load) {
    std::vector<double> weights(4);
    weights[0] = 1.0 / (1.0 + std::exp(-GAMMA * (load - DELTA_C))); // alpha_c
    weights[1] = 1.0 / (1.0 + std::exp(-GAMMA * (load - DELTA_C - 0.1))); // alpha_r
    weights[2] = 1.0 / (1.0 + std::exp(-GAMMA * (load - DELTA_C - 0.2))); // alpha_tr
    weights[3] = 1.0 / (1.0 + std::exp(-GAMMA * (load - DELTA_C - 0.3))); // alpha_p
    double sum = std::accumulate(weights.begin(), weights.end(), 0.0);
    for (auto& weight : weights) {
        weight /= sum;
    }
    return weights;
}

// Main algorithm loop simulating dynamic scenario over time slots
void main_algorithm(int T, std::vector<ServiceRequest>& requests, std::vector<RSU>& rsus, std::vector<PrefetchedService>& services) {
    DecisionVariables decisions;
    std::vector<double> weights;

    // Number generator to simulate variations over time
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.1, 0.3);  // Vary parameters like load and costs slightly to simulate realtime scenarios.

    double totalOverallLatency = 0.0;  // To accumulate the overall latency over time slots

    for (int t = 0; t < T; ++t) {
        // Simulate varying request loads and RSU parameters over time
        for (auto& request : requests) {
            double y = dis(gen);
            request.computationLoad *= y;  // Adjusting computation load
            request.transferCost *= y;    // Adjusting transfer cost
        }

        for (auto& rsu : rsus) {
            double y = dis(gen);
            rsu.computationCost *= dis(gen);  // Adjusting computation cost
            rsu.retentionCost *= dis(gen);    // Adjusting retention cost
        }

        // Compute system load
        double totalCapacity = std::accumulate(rsus.begin(), rsus.end(), 0.0, [](double sum, const RSU& rsu) {
            return sum + rsu.maxCapacity;
        });
        double usedCapacity = std::accumulate(rsus.begin(), rsus.end(), 0.0, [](double sum, const RSU& rsu) {
            return sum + rsu.usedCapacity;
        });
        double load = usedCapacity / totalCapacity;

        // Update dynamic weights
        weights = computeDynamicWeights(load);

        // Prefetch services (just a simulation, no need to output anything)
        for (auto& rsu : rsus) {
            double remainingCapacity = rsu.maxCapacity - rsu.usedCapacity;
            for (auto& service : services) {
                if (service.size <= remainingCapacity) {
                    decisions.P[service.id] = 1; // Prefetch service
                    remainingCapacity -= service.size;
                    rsu.usedCapacity += service.size;
                }
            }
        }

        // Record start time of request scheduling
        auto startScheduling = std::chrono::high_resolution_clock::now();

        // Schedule requests (without any output)
        for (auto& request : requests) {
            double minCost = std::numeric_limits<double>::max();
            int bestRSU = -1;

            for (auto& rsu : rsus) {
                if (rsu.usedCapacity + request.computationLoad <= rsu.maxCapacity) {
                    double cost = weights[0] * rsu.computationCost * request.computationLoad +
                                 weights[1] * rsu.retentionCost +
                                 weights[2] * request.transferCost +
                                 weights[3] * request.preparationCost;

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

        // Measure scheduling latency
        auto endScheduling = std::chrono::high_resolution_clock::now();
        double schedulingLatency = std::chrono::duration<double, std::micro>(endScheduling - startScheduling).count(); // in microseconds

        // Transfer requests (without any output)
        for (auto& request : requests) {
            double minTransferCost = std::numeric_limits<double>::max();
            int bestRSU = -1;

            for (auto& rsu : rsus) {
                if (rsu.usedCapacity + request.demand <= rsu.maxCapacity) {
                    double workloadPenalty = rsu.usedCapacity / rsu.maxCapacity;
                    double transferCost = request.distanceToRSU + TRANSFER_COST_MULTIPLIER * workloadPenalty;
                    if (transferCost < minTransferCost) {
                        minTransferCost = transferCost;
                        bestRSU = rsu.id;
                    }
                }
            }

            if (bestRSU != -1) {
                decisions.T[request.id] = bestRSU;
                rsus[bestRSU].usedCapacity += request.demand;
            }
        }

        // Compute total cost and total latency (including request scheduling latency)
        double totalCost = 0.0;
        double totalLatency = 0.0;

        for (const auto& request : requests) {
            int assignedRSU = decisions.X.at(request.id);
            const auto& rsu = rsus[assignedRSU];

            totalCost += rsu.computationCost * request.computationLoad +
                         rsu.retentionCost +
                         request.transferCost +
                         request.preparationCost;

            totalLatency += request.computationLoad * rsu.computationCost;
            totalLatency += request.transferCost;
        }

        for (const auto& service : services) {
            if (decisions.P[service.id] == 1) {
                totalCost += PREFETCH_COST_MULTIPLIER * service.prefetchCost;
            }
        }

        // Add the scheduling latency to the total latency
        totalLatency += schedulingLatency;

        // Add scheduling latency to the overall latency
        totalOverallLatency += schedulingLatency;

        // Output total cost and total latency for the current time slot
        std::cout << "Time Slot " << t << ": Total Cost = " << totalCost << std::endl;
        std::cout << "Time Slot " << t << ": Total Latency = " << totalLatency << " microseconds" << std::endl;
        // std::cout << "Time Slot " << t << ": Request Scheduling Latency = " << schedulingLatency << " microseconds" << std::endl;
    }

    // Output the overall latency
    std::cout << "Overall Latency across all time slots: " << totalOverallLatency << " microseconds" << std::endl;
}

int main() {
    std::vector<RSU> rsus = {
        {0, 110.0, 0.0, 0.02, 0.03, 0.01},
        {1, 120.0, 0.0, 0.04, 0.02, 0.025},
        {2, 130.0, 0.0, 0.025, 0.05, 0.02}
    };

    std::vector<ServiceRequest> requests = {
        {0, 4.0, 25.0, 0.025, 0.02, 10.0, 110.0},
        {1, 5.0, 35.0, 0.035, 0.02, 15.0, 130.0},
        {2, 2.0, 12.0, 0.015, 0.008, 5.0, 90.0}
    };

    std::vector<PrefetchedService> services = {
        {0, 10.0, 2.0},
        {1, 15.0, 3.0},
        {2, 8.0, 1.5}
    };

    int T = 5; // Number of time slots

    main_algorithm(T, requests, rsus, services);

    return 0;
}
