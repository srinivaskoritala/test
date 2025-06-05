# Luxor Cloud Network Provisioning Service - Design Document

## API Design Rationale

### Resource Model
The API is designed around two primary resources:
1. **Networks**: Representing VXLAN overlay networks
2. **Endpoints**: Representing VM interfaces connected to these networks

### Key Design Decisions

1. **RESTful Resource Hierarchy**
   - Networks are top-level resources
   - Endpoints are nested under networks
   - This hierarchy naturally represents the relationship between networks and their endpoints
   - Enables efficient querying of endpoints within a network

2. **UUID-based Resource Identification**
   - Using UUIDs for resource IDs ensures global uniqueness
   - Enables distributed systems to generate IDs without coordination
   - Makes it easier to implement caching and load balancing

3. **Comprehensive Resource Properties**
   - Networks include VNI (VXLAN Network Identifier)
   - Endpoints include MAC address, IP address, and VTEP information
   - Timestamps for auditing and state tracking

4. **Error Handling**
   - Consistent error response format
   - HTTP status codes aligned with REST conventions
   - Detailed error messages for debugging

## Implementation Choices

### Language Selection: C
- **Rationale**:
  - Direct interaction with Linux networking subsystem
  - High performance for network operations
  - Low-level control over system resources
  - Efficient memory management
  - Small memory footprint

### Key Libraries
1. **libmicrohttpd**
   - Lightweight HTTP server library
   - Good performance characteristics
   - Simple integration with C codebase

2. **json-c**
   - JSON parsing and generation
   - Well-maintained and stable
   - Good performance for our use case

### Linux Networking Integration
We've chosen Option A (generating iproute2 commands) for the following reasons:
1. **Simplicity**: Easier to implement and debug
2. **Transparency**: Commands clearly show what would happen
3. **Flexibility**: Easy to modify or extend
4. **Documentation**: Commands serve as documentation

## Scalability Considerations

### State Management
1. **In-Memory Storage (PoC)**
   - Simple hash tables for networks and endpoints
   - O(1) lookup time for resources
   - Thread-safe operations

2. **Future Evolution**
   - Distributed key-value store (e.g., etcd)
   - Sharding based on tenant_id
   - Caching layer for frequently accessed data

### Performance Optimizations
1. **Connection Pooling**
   - Reuse HTTP connections
   - Keep-alive connections
   - Connection limits per client

2. **Response Caching**
   - Cache network and endpoint details
   - Invalidation on updates
   - TTL-based cache expiration

## Security Considerations

### API Security
1. **Authentication**
   - JWT-based authentication
   - API key support for service-to-service communication
   - Role-based access control

2. **Authorization**
   - Tenant isolation
   - Resource-level permissions
   - Audit logging

### Network Security
1. **VXLAN Security**
   - VNI isolation
   - MAC address filtering
   - VTEP authentication

2. **Data Plane Security**
   - Encryption for VXLAN tunnels
   - Rate limiting
   - DDoS protection

## Control Plane Integration

### SDN Architecture
1. **BGP EVPN Integration**
   - VNI to VRF mapping
   - Route distribution
   - MAC address learning

2. **Controller Communication**
   - REST API for northbound interface
   - gRPC for east-west communication
   - Event-driven updates

## Assumptions & Trade-offs

### Assumptions
1. Single data center deployment for PoC
2. Linux-based hosts
3. VXLAN as the overlay technology
4. IPv4 addressing

### Trade-offs
1. **Simplicity vs. Features**
   - Focused on core functionality
   - Deferred advanced features
   - Clear upgrade path

2. **Performance vs. Flexibility**
   - Optimized for common operations
   - Extensible design
   - Room for future optimizations

3. **Development Speed vs. Production Readiness**
   - PoC implementation
   - Production-grade API design
   - Clear path to production

## Future Considerations

1. **Multi-Datacenter Support**
   - Cross-DC VXLAN tunnels
   - Geographic routing
   - Disaster recovery

2. **Advanced Features**
   - QoS policies
   - ACLs
   - Service chaining

3. **Monitoring & Observability**
   - Metrics collection
   - Health checks
   - Performance monitoring 