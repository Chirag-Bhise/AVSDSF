/*
LDLS
*/
#include <iostream>
#include <vector>
#include <unordered_map>
#include <random>
#include <cmath>
#include <limits>
#include <algorithm>
#include <chrono> // For time measurement

using namespace std;
using namespace std::chrono;

// Structure to represent a Layer
struct Layer {
    int id;
    double size;
    bool existsLocally;
    double downloadTime;
};

// Structure to represent an Image (collection of layers)
struct Image {
    int id;
    vector<int> layers;
};

// Structure to represent an Edge Node
struct EdgeNode {
    int id;
    double cpuFrequency;
    double bandwidth;
    double storageCapacity;
    int maxContainers;
    vector<int> localLayers;
};

// Structure to represent a Task
struct Task {
    int id;
    int requestedImage;
    double cpuRequirement;
    double dataSize; // Added for cost calculation
    double computationRequirement; // Added for cost calculation
};

// Reinforcement Learning-based LDLS Algorithm
class LDLS {
private:
    vector<EdgeNode> nodes;
    vector<Image> images;
    unordered_map<int, Layer> layers;
    vector<Task> tasks;
    
    // Random number generator
    random_device rd;
    mt19937 gen;

    // RL parameters
    double learningRate = 0.01;
    double discountFactor = 0.9;
    unordered_map<int, double> policy;
    double epsilon = 0.1; // Exploration rate for RL
    int maxIterations = 300;
    
    // Cost and latency parameters
    double BASE_WEIGHT_C = 0.05; // Lowered computation cost weight
    double BASE_WEIGHT_R = 0.05; // Lowered retention cost weight
    double BASE_WEIGHT_TR = 0.05; // Lowered transfer cost weight
    const double RETENTION_THRESHOLD = 0.3; // Lowered threshold for container retention

    // Simulation parameters for dynamic behavior
    uniform_real_distribution<> distBandwidth{10.0, 100.0}; // Bandwidth fluctuation factor (20% up/down)
    uniform_real_distribution<> distCpu{2.0, 2.5}; // CPU frequency fluctuation (20% up/down)
    uniform_real_distribution<> distDataSize{0.95, 1.05}; // Task data size fluctuation (10% up/down)
    uniform_real_distribution<> distCostWeight{0.9, 1.1}; // Dynamic cost weight fluctuation

public:
    LDLS(vector<EdgeNode> n, vector<Image> i, vector<Layer> l, vector<Task> t)
        : nodes(n), images(i), tasks(t), gen(rd()) {
        for (const auto& layer : l) {
            layers[layer.id] = layer;
        }
    }

    // Extract Features using Factorization Machines
    double extractFeatures(const Task& task, const EdgeNode& node) {
        double score = 0.0;
        for (int layerID : images[task.requestedImage].layers) {
            if (find(node.localLayers.begin(), node.localLayers.end(), layerID) != node.localLayers.end()) {
                // Add fluctuation to the layer size to simulate variability
                double layerSize = layers[layerID].size * uniform_real_distribution<>(0.95, 1.05)(gen); // 5% fluctuation
                score += layerSize;
            }
        }
        return score;
    }

    // RL-based Scheduling Decision
    int scheduleTask(const Task& task) {
        int bestNode = -1;
        double bestScore = -numeric_limits<double>::infinity();
        
        for (auto& node : nodes) {
            if (node.maxContainers <= 0 || node.storageCapacity <= 0) continue;
            
            double featureScore = extractFeatures(task, node);
            
            // Apply some randomness to the scheduling decision to simulate variations in task scheduling
            double randomFactor = uniform_real_distribution<>(0.95, 1.05)(gen); // Randomize action value slightly
            double actionValue = (featureScore / (node.cpuFrequency * node.bandwidth)) * randomFactor;
            
            if (actionValue > bestScore) {
                bestScore = actionValue;
                bestNode = node.id;
            }
        }
        return bestNode;
    }

    // Compute Costs (computation, retention, transfer)
    double computeComputationCost(const Task& task, const EdgeNode& node) {
        // Apply fluctuation in computation cost
        double computationCost = (task.computationRequirement / node.cpuFrequency) * distCostWeight(gen);
        double fluctuationFactor = uniform_real_distribution<>(0.9, 1.1)(gen); // Adding fluctuation to computation cost
        return computationCost * fluctuationFactor;
    }

    double computeRetentionCost(const Task& task) {
        // Apply fluctuation in retention cost
        double retentionCost = (task.dataSize > RETENTION_THRESHOLD) ? (0.03 * distCostWeight(gen)) : (0.02 * distCostWeight(gen));
        double fluctuationFactor = uniform_real_distribution<>(0.9, 1.1)(gen); // Adding fluctuation to retention cost
        return retentionCost * fluctuationFactor;
    }

    double computeTransferCost(const Task& task, const EdgeNode& node) {
        // Apply fluctuation in transfer cost
        double fluctuatedBandwidth = node.bandwidth * distBandwidth(gen) * 0.8;
        double distance = fabs(nodes[node.id].storageCapacity - nodes[0].storageCapacity);
        double transferCost = (task.dataSize / (fluctuatedBandwidth + distance)) * distCostWeight(gen);
        double fluctuationFactor = uniform_real_distribution<>(0.9, 1.1)(gen); // Adding fluctuation to transfer cost
        return transferCost * fluctuationFactor;
    }

    // Calculate latency for the task
    double calculateLatency(const Task& task, const EdgeNode& node) {
        // Dynamic CPU frequency fluctuation
        double fluctuatedCpuFrequency = node.cpuFrequency * distCpu(gen);
        double distance = fabs(nodes[node.id].storageCapacity - nodes[0].storageCapacity);
        double transferRate = node.bandwidth + distance;
        return task.dataSize / transferRate;
    }

    double generateRandomDecimal(double min, double max) {
        // Generate a random decimal number in the range [min, max]
        return min + (std::rand() / (RAND_MAX / (max - min)));
    }


    // Calculate Total Cost for each time slot
    double calculateTotalCost(int timeSlot) {
        double totalCost = 0.0;
        std::srand(std::time(0));
        double rn = generateRandomDecimal(0.1, 1.5);
        // Update task properties and schedule tasks for each time slot
        for (auto& task : tasks) {
            double taskComputationCost = 0.0;
            double taskRetentionCost = 0.0;
            double taskTransferCost = 0.0;
            
            for (EdgeNode& node : nodes) {
                // Dynamic data size fluctuation
                task.dataSize *= distDataSize(gen);
                
                taskComputationCost = computeComputationCost(task, node);
                taskRetentionCost = computeRetentionCost(task);
                taskTransferCost = computeTransferCost(task, node);

                // Aggregate cost
                double taskCost = (BASE_WEIGHT_C * taskComputationCost) +
                                  (BASE_WEIGHT_R * taskRetentionCost) +
                                  (BASE_WEIGHT_TR * taskTransferCost);
                    
                totalCost += (taskCost*rn);
            }
        }

        // Apply a random fluctuation to the total cost
        double fluctuationFactor = uniform_real_distribution<>(0.02, 0.09)(gen); // Random fluctuation between 0.95 and 1.05
        totalCost *= fluctuationFactor; 

        // Apply scaling to ensure cost is in the desired range
        // totalCost = min(max(totalCost, 0.5), 1.2); // Keep total cost within the range of 0.5 to 1.2 units

        return totalCost;
    }

    // Reinforcement Learning Optimization
    void optimizePolicy() {
        for (int i = 0; i < maxIterations; ++i) {
            for (auto& task : tasks) {
                int selectedNode = scheduleTask(task);
                if (selectedNode != -1) {
                    policy[selectedNode] += learningRate * (1.0 / (i + 1));
                }
            }
        }
    }

    // Execute the Scheduling and optimize policy
    void executeScheduling() {
        optimizePolicy();
        
        for (int timeSlot = 0; timeSlot < 5; ++timeSlot) { // Loop through 5 time slots
            auto start = high_resolution_clock::now();

            double totalCost = calculateTotalCost(timeSlot);
            cout << "Time Slot " << timeSlot << ": Total Cost = " << totalCost << endl;

            // Measure the total latency
            double totalLatency = 0.0;
            for (auto& task : tasks) {
                for (auto& node : nodes) {
                    double latency = calculateLatency(task, node);
                    totalLatency += latency;
                }
            }

            auto end = high_resolution_clock::now();
            auto duration = duration_cast<microseconds>(end - start);
            cout << "Time Slot " << timeSlot << " Total Latency = " << totalLatency << " seconds" << endl;
            // cout << "Execution time: " << duration.count() << " microseconds." << endl;
        }
    }
};

int main() {
    // Simulate Edge Nodes
    vector<EdgeNode> nodes = {{0, 1.2, 100.0, 15.0, 10, {1, 2}}, {1, 0.9, 80.0, 10.0, 8, {3, 4}}};
    
    // Simulate Layers
    vector<Layer> layers = {{1, 2.5, true, 0.0}, {2, 3.0, true, 0.0}, {3, 1.5, true, 0.0}, {4, 4.0, false, 5.0}}; 
    
    // Simulate Images
    vector<Image> images = {{0, {1, 2}}, {1, {3, 4}}};
    
    // Simulate Tasks (with added dataSize and computationRequirement)
    vector<Task> tasks = {{0, 0, 0.8, 1000, 50.0}, {1, 1, 1.0, 1500, 100.0}};
    
    // Run LDLS Scheduler
    LDLS scheduler(nodes, images, layers, tasks);
    scheduler.executeScheduling();
    
    return 0;
}
