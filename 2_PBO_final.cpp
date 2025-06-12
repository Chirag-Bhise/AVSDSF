/*
Pressure Based Optimization
*/
#include <iostream>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <chrono> // For measuring execution time
#include <random> // Include the random library for introducing randomness

using namespace std;
using namespace std::chrono;

// Structure to represent a Compute Unit (Edge/Cloud Server)
struct ComputeUnit {
    string id;
    double cpu_usage;         // Resource pressure
    double network_latency;   // Latency to other compute units
    int function_replicas;    // Number of running function instances
    int max_capacity;         // Maximum function instances this unit can handle
};

// Structure to represent a Serverless Function
struct FunctionInstance {
    string id;
    ComputeUnit* host;
};

// Cost and Latency Parameters
const double COMPUTATION_COST_WEIGHT = 0.3;
const double TRANSFER_COST_WEIGHT = 0.3;
const double RETENTION_COST_WEIGHT = 0.1;
const double LATENCY_WEIGHT = 0.4;
const double RETENTION_THRESHOLD = 0.5;

// Pressure Calculation Functions
double calculateRequestPressure(int request_count, int max_requests) {
    return (double)request_count / max_requests;
}

double calculatePerformancePressure(double rtt, double target_rtt) {
    return 1 / (1 + exp(-0.2 * (rtt - target_rtt))); // Logistic function for RTT pressure
}

double calculateResourcePressure(double cpu_usage, double max_cpu) {
    return cpu_usage / max_cpu;
}

// Compute final pressure value
double computePressure(double pREQ, double pRTT, double pRES) {
    return pREQ * pRTT * pRES;
}

// Function to compute computation cost
double computeComputationCost(double computationRequirement, double computationPower) {
    return computationRequirement / computationPower;
}

// Function to compute retention cost
double computeRetentionCost(double dataSize) {
    return (dataSize > RETENTION_THRESHOLD) ? 0.1 : 0.05;
}

// Function to compute transfer cost
double computeTransferCost(double dataSize, double networkLatency) {
    return dataSize / (networkLatency + 1); // Avoid division by zero
}

// Function to compute total latency
double computeLatency(double dataSize, double transferRate) {
    return dataSize / transferRate;
}

// Scaling Function Based on Pressure
void scaleFunctions(vector<ComputeUnit>& units, double threshold_max, double threshold_min) {
    for (auto& unit : units) {
        double pREQ = calculateRequestPressure(unit.function_replicas, unit.max_capacity);
        double pRTT = calculatePerformancePressure(unit.network_latency, 70.0);
        double pRES = calculateResourcePressure(unit.cpu_usage, 100.0);
        double pressure = computePressure(pREQ, pRTT, pRES);
        
        if (pressure > threshold_max && unit.function_replicas < unit.max_capacity) {
            // cout << "Scaling UP on: " << unit.id << endl;
            unit.function_replicas++;
        } else if (pressure < threshold_min && unit.function_replicas > 1) {
            // cout << "Scaling DOWN on: " << unit.id << endl;
            unit.function_replicas--;
        }
    }
}

// Placement Decision: Find the Best Compute Unit for Deployment
ComputeUnit* findBestPlacement(vector<ComputeUnit>& units, double threshold_max) {
    ComputeUnit* bestUnit = nullptr;
    double lowestPressure = threshold_max;

    for (auto& unit : units) {
        if (unit.function_replicas < unit.max_capacity) {
            double pREQ = calculateRequestPressure(unit.function_replicas, unit.max_capacity);
            double pRTT = calculatePerformancePressure(unit.network_latency, 70.0);
            double pRES = calculateResourcePressure(unit.cpu_usage, 100.0);
            double pressure = computePressure(pREQ, pRTT, pRES);

            if (pressure < lowestPressure) {
                lowestPressure = pressure;
                bestUnit = &unit;
            }
        }
    }

    return bestUnit;
}

// Router Optimization: Load Balancing Based on Latency & Resources
void optimizeRouting(vector<ComputeUnit>& units, unordered_map<string, vector<FunctionInstance>>& functionMap) {
    for (auto& [funcId, instances] : functionMap) {
        double totalWeight = 0;
        unordered_map<ComputeUnit*, double> weights;

        for (auto& instance : instances) {
            double latencyFactor = max(0.01, 1 / (1 + exp(-0.2 * (instance.host->network_latency - 35.0))));
            double cpuFactor = 1 - (instance.host->cpu_usage / 100.0);
            double weight = (latencyFactor * cpuFactor) * 100;

            weights[instance.host] = weight;
            totalWeight += weight;
        }

        // cout << "Routing Weights for Function " << funcId << ":\n";
        for (auto& [unit, weight] : weights) {
            // cout << "  " << unit->id << " -> " << (weight / totalWeight) * 100 << "% traffic\n";
        }
    }
}

// Function to simulate time slots and measure performance
void simulateTimeSlots(vector<ComputeUnit>& units, unordered_map<string, vector<FunctionInstance>>& functionMap, int numSlots) {
    random_device rd;
    mt19937 gen(rd()); // Mersenne Twister random number generator
    uniform_real_distribution<> dis(0.01, 0.05); // Uniform distribution for small fluctuations (5% range)

    for (int timeSlot = 0; timeSlot < numSlots; ++timeSlot) {
        

        cout << "\n--- Time Slot " << timeSlot << " ---\n";

        auto start = high_resolution_clock::now(); // Start time measurement
        // Scale functions
        scaleFunctions(units, 0.5, 0.1);

        // Placement decisions
        ComputeUnit* bestUnit = findBestPlacement(units, 0.5);
        if (bestUnit) {
            // functionMap["funcA"].push_back({"inst_new", bestUnit});
            // cout << "New Function Instance placed on: " << bestUnit->id << endl;
        }

        // Optimize routing
        optimizeRouting(units, functionMap);

        // Compute total cost and latency
        double totalCost = 0.0;
        double totalLatency = 0.0;

        for (auto& [funcId, instances] : functionMap) {
            for (auto& instance : instances) {
                // Introduce randomness into cost and latency calculations
                double computationCost = computeComputationCost(1000, instance.host->cpu_usage) * dis(gen);
                double retentionCost = computeRetentionCost(0.02) * dis(gen);
                double transferCost = computeTransferCost(0.02, instance.host->network_latency) * dis(gen);
                double latency = computeLatency(0.02, instance.host->network_latency + 50) * dis(gen);

                double cost = (COMPUTATION_COST_WEIGHT * computationCost) +
                              (RETENTION_COST_WEIGHT * retentionCost) +
                              (TRANSFER_COST_WEIGHT * transferCost) +
                              (LATENCY_WEIGHT * latency);

                totalCost += cost;
                totalLatency += latency;
            }
        }

        cout << "Total Cost: " << totalCost << endl;
        cout << "Total Latency: " << totalLatency * 1000000 << " microseconds" << endl; // Latency in microseconds

        auto end = high_resolution_clock::now(); // End time measurement
        auto duration = duration_cast<microseconds>(end - start);
        cout << "Execution Time: " << duration.count() << " microseconds.\n"; // Execution time in microseconds
    }
}

int main() {
    // Example Compute Units
    vector<ComputeUnit> compute_units = {
        {"Edge-1", 30.0, 50.0, 3, 10},
        {"Edge-2", 40.0, 60.0, 2, 10},
        {"Cloud", 70.0, 150.0, 5, 20}
    };

    // Serverless Functions and Instances
    unordered_map<string, vector<FunctionInstance>> functionInstances;
    functionInstances["funcA"] = {{"inst1", &compute_units[0]}, {"inst2", &compute_units[1]}};

    // Simulate time slots and performance measurement
    simulateTimeSlots(compute_units, functionInstances, 5);

    return 0;
}
