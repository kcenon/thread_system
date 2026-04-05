# Ecosystem Integration

## Position in kcenon Ecosystem

**thread_system** is the concurrency layer of the kcenon ecosystem. It provides high-performance thread pools and DAG-based task scheduling used across systems that require parallel execution.

## Dependencies

| System | Relationship |
|--------|-------------|
| common_system | Direct dependency — core interfaces and patterns |

## Dependent Systems

| System | How It Uses thread_system |
|--------|--------------------------|
| monitoring_system | Thread pool for metric collectors and background processing |

## All Ecosystem Systems

| System | Description | Docs |
|--------|------------|------|
| common_system | Foundation — interfaces, patterns, utilities | [Docs](https://kcenon.github.io/common_system/) |
| thread_system | High-performance thread pool, DAG scheduling | [Docs](https://kcenon.github.io/thread_system/) |
| logger_system | Async logging, decorators, OpenTelemetry | [Docs](https://kcenon.github.io/logger_system/) |
| container_system | Type-safe containers, SIMD serialization | [Docs](https://kcenon.github.io/container_system/) |
| monitoring_system | Metrics, tracing, alerts, plugins | [Docs](https://kcenon.github.io/monitoring_system/) |
| database_system | Multi-backend DB (PostgreSQL, SQLite, MongoDB, Redis) | [Docs](https://kcenon.github.io/database_system/) |
| network_system | TCP/UDP/WebSocket/HTTP2/QUIC/gRPC networking | [Docs](https://kcenon.github.io/network_system/) |
| pacs_system | DICOM medical imaging (12 libraries) | [Docs](https://kcenon.github.io/pacs_system/) |

## Ecosystem Overview

See the [Ecosystem Overview](https://kcenon.github.io/common_system/) in common_system for the complete dependency map and system selection guide.
