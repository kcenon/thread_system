---
doc_id: "THR-ARCH-003"
doc_title: "Thread System - Architecture Diagrams & Visual Reference"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "thread_system"
category: "ARCH"
---

# Thread System - Architecture Diagrams & Visual Reference

> **SSOT**: This document is the single source of truth for **Thread System - Architecture Diagrams & Visual Reference**.

This guide has been split into two focused documents for easier navigation:

| Document | Description | Sections |
|----------|-------------|----------|
| **[Architecture Overview](ARCHITECTURE_OVERVIEW.md)** | High-level architecture | System architecture, core module hierarchy, threading & job execution flow |
| **[Architecture Details](ARCHITECTURE_DETAILS.md)** | Component deep-dives | Queue architecture, queue implementations, typed thread pool, hazard pointers, cancellation tokens, error handling |

## Quick Links

### Overview
- [System Architecture Overview](ARCHITECTURE_OVERVIEW.md#1-system-architecture-overview)
- [Core Module Component Hierarchy](ARCHITECTURE_OVERVIEW.md#2-core-module-component-hierarchy)
- [Threading & Job Execution Flow](ARCHITECTURE_OVERVIEW.md#3-threading--job-execution-flow)

### Details
- [Queue Capabilities Architecture](ARCHITECTURE_DETAILS.md#4-queue-capabilities-architecture)
- [Queue Implementation Strategy Comparison](ARCHITECTURE_DETAILS.md#5-queue-implementation-strategy-comparison)
- [Type-Based Thread Pool Architecture](ARCHITECTURE_DETAILS.md#6-type-based-thread-pool-architecture)
- [Hazard Pointer Memory Reclamation Flow](ARCHITECTURE_DETAILS.md#7-hazard-pointer-memory-reclamation-flow)
- [Cancellation Token Hierarchy](ARCHITECTURE_DETAILS.md#8-cancellation-token-hierarchy)
- [Error Handling Flow](ARCHITECTURE_DETAILS.md#9-error-handling-flow)
