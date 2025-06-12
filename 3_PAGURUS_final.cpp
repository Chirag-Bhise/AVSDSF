/*
PAGURUS
*/
#include <iostream>
#include <vector>
#include <unordered_map>
#include <queue>
#include <random>
#include <chrono>
#include <set>
#include <iomanip>

// Structure to represent a function container
struct Container {
    std::string functionName;
    std::string type; // "private", "zygote", "helper"
    bool isIdle;
    
    Container(std::string name, std::string t, bool idle) : functionName(name), type(t), isIdle(idle) {}
};

class PagurusManager {
private:
    std::unordered_map<std::string, std::vector<Container>> functionContainers; // Map of function name to its containers
    std::unordered_map<std::string, std::set<std::string>> functionDependencies; // Tracks function dependencies
    std::vector<double> costPerSlot; // Tracks costs for each time slot
    std::vector<double> latencies; // Tracks latencies for each time slot
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<double> costVariation;

public:
    PagurusManager() : costPerSlot(5, 0.0), latencies(5, 0.0), gen(rd()), costVariation(0.1, 0.3) {}

    // Identify idle containers and convert them to zygote
    void identifyIdleContainers(int timeSlot, std::chrono::time_point<std::chrono::high_resolution_clock>& slotStartTime) {
        double cost = 0.0;
        auto start = std::chrono::high_resolution_clock::now();  // Start latency measurement
        for (auto& func : functionContainers) {
            for (auto& container : func.second) {
                if (container.isIdle && container.type == "private") {
                    container.type = "zygote"; // Convert idle private container to zygote
                    double dynamicCost = 0.1 + costVariation(gen);
                    cost += dynamicCost;
                }
            }
        }
        costPerSlot[timeSlot] += cost;
        auto end = std::chrono::high_resolution_clock::now();  // End latency measurement
        latencies[timeSlot] += std::chrono::duration<double, std::micro>(end - start).count(); // Update latency in microseconds for this slot
    }

    // Function to fork a zygote container into a helper container
    void forkZygote(std::string functionName, std::string targetFunction, int timeSlot, std::chrono::time_point<std::chrono::high_resolution_clock>& slotStartTime) {
        double cost = 0.0;
        auto start = std::chrono::high_resolution_clock::now();  // Start latency measurement
        for (auto& container : functionContainers[functionName]) {
            if (container.type == "zygote") {
                functionContainers[targetFunction].push_back(Container(targetFunction, "helper", false));
                double dynamicCost = 0.05 + costVariation(gen);
                cost += dynamicCost;
                costPerSlot[timeSlot] += cost;
                break;
            }
        }
        auto end = std::chrono::high_resolution_clock::now();  // End latency measurement
        latencies[timeSlot] += std::chrono::duration<double, std::micro>(end - start).count(); // Update latency in microseconds for this slot
    }

    // Implementing SF-WRS selection
    std::string selectFunctionToHelp(std::string functionName) {
        std::vector<std::string> candidates;
        for (const auto& func : functionDependencies[functionName]) {
            candidates.push_back(func);
        }
        if (candidates.empty()) return "";

        std::uniform_int_distribution<> dis(0, candidates.size() - 1);
        return candidates[dis(gen)];
    }

    // Load balancer to distribute functions efficiently
    void balanceFunctions(int timeSlot, std::chrono::time_point<std::chrono::high_resolution_clock>& slotStartTime) {
        double dynamicCost = 0.05 + costVariation(gen);
        costPerSlot[timeSlot] += dynamicCost;
    }

    // Function to add a new container
    void addContainer(std::string functionName, std::string type) {
        functionContainers[functionName].push_back(Container(functionName, type, true));
    }

    // Establish function dependencies to enable helper containers
    void setupFunctionDependencies() {
        functionDependencies["FunctionA"].insert("FunctionB"); // FunctionA can help FunctionB
        functionDependencies["FunctionB"].insert("FunctionA"); // FunctionB can help FunctionA
    }

    // Simulating function invocation and container utilization
    void simulateFunctionInvocation(std::string functionName, int timeSlot, std::chrono::time_point<std::chrono::high_resolution_clock>& slotStartTime) {
        double cost = 0.0;
        auto start = std::chrono::high_resolution_clock::now();  // Start latency measurement
        for (auto& container : functionContainers[functionName]) {
            if (!container.isIdle) {
                double dynamicCost = 0.02 + costVariation(gen);
                cost += dynamicCost;
                costPerSlot[timeSlot] += cost;
                auto end = std::chrono::high_resolution_clock::now();  // End latency measurement
                latencies[timeSlot] += std::chrono::duration<double, std::micro>(end - start).count(); // Update latency in microseconds for this slot
                return;
            }
        }
        std::string helperFunction = selectFunctionToHelp(functionName);
        if (!helperFunction.empty()) {
            forkZygote(helperFunction, functionName, timeSlot, slotStartTime);
        } else {
            double dynamicCost = 0.3 + costVariation(gen);
            addContainer(functionName, "private");
            cost += dynamicCost;
        }
        costPerSlot[timeSlot] += cost;
        auto end = std::chrono::high_resolution_clock::now();  // End latency measurement
        latencies[timeSlot] += std::chrono::duration<double, std::micro>(end - start).count(); // Update latency in microseconds for this slot
    }

    // Display cost and latency per time slot
    void displayCostsAndLatencies() {
        for (size_t i = 0; i < costPerSlot.size(); ++i) {
            std::cout << "Time Slot " << i << ": Total Cost = " << std::fixed << std::setprecision(6) << costPerSlot[i]
                      << ", Latency = " << latencies[i] << " microseconds" << std::endl;
        }
    }
};

int main() {
    PagurusManager manager;
    manager.setupFunctionDependencies();
    manager.addContainer("FunctionA", "private");
    manager.addContainer("FunctionB", "private");

    auto start = std::chrono::high_resolution_clock::now();
    for (int timeSlot = 0; timeSlot < 5; ++timeSlot) {
        auto slotStartTime = std::chrono::high_resolution_clock::now(); // Start time for this time slot
        manager.identifyIdleContainers(timeSlot, slotStartTime);
        manager.simulateFunctionInvocation("FunctionA", timeSlot, slotStartTime);
        manager.simulateFunctionInvocation("FunctionB", timeSlot, slotStartTime);
        manager.balanceFunctions(timeSlot, slotStartTime);
    }
    auto end = std::chrono::high_resolution_clock::now();  // Get end time

    std::chrono::duration<double> duration = end - start;  // Calculate the total duration

    manager.displayCostsAndLatencies();
    // std::cout << "Total time taken for the entire operation: " << duration.count() << " seconds" << std::endl;
    return 0;
}
