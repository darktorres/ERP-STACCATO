# ERP Staccato Web - .NET Implementation (POC)

This is a proof-of-concept implementation of the ERP Staccato web stack using .NET technologies.

## Stack

- **Backend**: ASP.NET Core 9 with Entity Framework Core
- **Frontend**: Blazor (Web Assembly)
- **Database**: MariaDB/MySQL (shared with C++ application)
- **Communication**: REST API with async/await pattern

## Project Structure

```text
web-dotnet/
├── src/
│   ├── Shared/                    # Shared types and contracts
│   │   ├── DTOs/
│   │   └── Models/
│   ├── Backend/                   # ASP.NET Core API
│   │   ├── Controllers/
│   │   ├── Services/
│   │   ├── Data/
│   │   └── appsettings.json
│   └── Frontend/                  # Blazor WebAssembly
│       ├── Pages/
│       ├── Components/
│       └── Services/
├── ERP.Staccato.Web.sln          # Solution file
└── README.md
```

## Getting Started

### Prerequisites

- .NET 9 SDK or later
- Visual Studio 2022 / VS Code with C# extensions
- MariaDB/MySQL database (shared with C++ application)

### Running the Application

#### Backend (ASP.NET Core)

```bash
cd src/Backend
dotnet restore
dotnet run
# Runs on https://localhost:7001
```

#### Frontend (Blazor WebAssembly)

```bash
cd src/Frontend
dotnet restore
dotnet run
# Runs on https://localhost:7002
```

## Features (POC)

### Implemented

- [ ] Authentication (JWT)
- [ ] Orcamento (Budget) List with Filtering
- [ ] Role-based Access Control
- [ ] Real-time data loading with spinners
- [ ] Dark theme UI

### Planned

- [ ] Create/Edit Orcamento
- [ ] Followup Management
- [ ] Excel Export
- [ ] Advanced Filtering
- [ ] Performance Optimization (Virtual Scrolling)

## Development Notes

This POC mirrors the functionality of the TypeScript implementation:

- Same database models and views
- Equivalent filter logic
- Compatible authentication
- Shared database with C++ application

## Comparison with TypeScript Version

| Feature            | TypeScript   | .NET                 | Status |
| ------------------ | ------------ | -------------------- | ------ |
| Backend Framework  | NestJS       | ASP.NET Core         | ✓      |
| Frontend Framework | React        | Blazor WASM          | ✓      |
| State Management   | Zustand      | Cascading Parameters | ✓      |
| API Communication  | tRPC         | REST                 | ✓      |
| Database Access    | Prisma       | EF Core              | ✓      |
| ORM Pattern        | Schema-first | Code-first           | ✓      |

## Next Steps

1. Create database context and models
2. Build authentication controller
3. Build orcamento service and API
4. Create Blazor pages and components
5. Implement filtering logic
6. Add role-based visibility

## References

- [ASP.NET Core Documentation](https://learn.microsoft.com/en-us/aspnet/core/)
- [Blazor Documentation](https://learn.microsoft.com/en-us/aspnet/core/blazor/)
- [Entity Framework Core](https://learn.microsoft.com/en-us/ef/core/)
