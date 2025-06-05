# Luxor Cloud Network Provisioning Service

This project implements a dynamic tenant network provisioning service for Luxor Cloud, focusing on Layer 2 overlay networks using VXLAN technology.

## Project Structure

```
.
├── api/
│   └── openapi.yaml       # OpenAPI specification
├── src/
│   ├── main.c            # Main service entry point
│   ├── api/
│   │   ├── handlers.c    # API request handlers
│   │   └── handlers.h
│   ├── network/
│   │   ├── vxlan.c      # VXLAN network management
│   │   └── vxlan.h
│   ├── storage/
│   │   ├── memory.c     # In-memory state management
│   │   └── memory.h
│   └── utils/
│       ├── logging.c    # Logging utilities
│       └── logging.h
├── tests/               # Unit tests
├── Makefile            # Build configuration
└── DESIGN.md          # Detailed design document
```

## Building and Running

### Prerequisites

- GCC 9.0 or later
- libmicrohttpd
- json-c
- CMake 3.10 or later

### Build Instructions

```bash
# Clone the repository
git clone git@github.com:srinivaskoritala/test.git
cd luxor-network-service

# Build the project
make

# Run the service
./bin/network-service
```

The service will start on port 8080 by default.

## API Documentation

The API is documented using OpenAPI 3.0 specification in `api/openapi.yaml`. Key endpoints include:

- `POST /api/v1/networks` - Create a new tenant network
- `GET /api/v1/networks/{network_id}` - Get network details
- `POST /api/v1/networks/{network_id}/endpoints` - Add endpoint to network
- `GET /api/v1/networks/{network_id}/endpoints` - List network endpoints
- `DELETE /api/v1/networks/{network_id}/endpoints/{endpoint_id}` - Remove endpoint

## Design Decisions

See `DESIGN.md` for detailed explanations of:
- API design rationale
- Implementation choices
- Scalability considerations
- Performance aspects
- Security considerations
- Control plane integration
- Assumptions and trade-offs

## Testing

```bash
# Run unit tests
make test
```


