# AVSDSF: Autonomous Vehicle Services Deployment and Scheduling Framework

**C++ Implementation Repository for Research Work**

## Overview

Autonomous Vehicle Services Deployment and Scheduling Framework (**AVSDSF**) is a novel edge-computing framework designed to optimize the deployment and scheduling of AV service images in Intelligent Transportation Systems (ITS). This framework specifically addresses AV mobility, service latency, and edge resource utilization using a dual-algorithmic approach.

This repository contains the **C++ implementations** of the proposed algorithms as introduced in our research:

- **AVSDSF (Framework)**
  - **BS-PAD Algorithm**: Base Station Prefetching and AV Deployment
  - **RS-MAS Algorithm**: RSU Scheduling based on Mobility and AV Services

## Abstract

Serverless edge computing plays a key role in supporting latency-sensitive applications like Autonomous Vehicles (AVs). While previous works have examined scheduling, container retention, and offloading, the issues of **service image prefetching** and **mobility-aware deployment** remain largely unresolved.

To address these challenges, we propose the **AVSDSF framework**, which incorporates two novel algorithms:

1. **BS-PAD (Base Station Prefetching and AV Deployment)**:
   - Predictively identifies and deploys common AV service images.
   - Minimizes cloud interactions and service response times.

2. **RS-MAS (RSU Scheduling based on Mobility and AV Services)**:
   - Schedules AV service requests at RSUs using real-time mobility and demand awareness.
   - Balances resource usage across edge infrastructure.

Our framework, when compared to state-of-the-art solutions (ONCO, PBO, PAGURUS, MMTO, LDLS), achieved:
- **47.47%** higher AV service request handling success.
- **23.99%** higher success for high-priority AV services.
- **38.67%** reduction in image distribution time.

## Key Features

- **Dual-level intelligent algorithmic design**
- **Mobility-aware and service-driven edge deployment**
- **Reduced latency and cloud dependency**
- **Improved RSU resource fairness and load balancing**

## Repository Structure

```bash
├── base_station/               # BS-PAD Algorithm
│   └── bs_pad.cpp
├── roadside_unit/             # RS-MAS Algorithm
│   └── rs_mas.cpp
├── data/                      # Sample input datasets or logs
├── results/                   # Output logs and evaluation data
├── README.md
└── LICENSE
