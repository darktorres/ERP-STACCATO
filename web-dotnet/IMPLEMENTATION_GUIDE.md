# .NET Web Implementation Guide

## Overview

This is a proof-of-concept implementation of ERP Staccato using .NET technologies, mirroring the TypeScript implementation but with .NET backend and Blazor frontend.

## Project Structure

```text
web-dotnet/
├── src/
│   ├── Shared/                          # Shared types (DTOs, Models)
│   │   ├── Models/
│   │   │   ├── SessionUser.cs          # Authenticated user
│   │   │   ├── Orcamento.cs            # Budget models and filters
│   │   │   └── Auth.cs                 # Auth request/response
│   │   └── Shared.csproj
│   │
│   ├── Backend/                         # ASP.NET Core API
│   │   ├── Controllers/
│   │   │   ├── AuthController.cs       # Authentication endpoints
│   │   │   └── OrcamentoController.cs  # Budget endpoints
│   │   ├── Services/
│   │   │   ├── AuthService.cs          # Auth logic
│   │   │   └── OrcamentoService.cs     # Budget filtering & queries
│   │   ├── Data/
│   │   │   └── ApplicationDbContext.cs # EF Core context
│   │   ├── Program.cs                  # Startup configuration
│   │   ├── appsettings.json            # Configuration
│   │   └── Backend.csproj
│   │
│   ├── Frontend/                        # Blazor WebAssembly
│   │   ├── Pages/
│   │   │   ├── Login.razor             # Login page
│   │   │   ├── Dashboard.razor         # Main dashboard
│   │   │   └── Orcamentos.razor        # Budget list with filters
│   │   ├── Components/
│   │   │   ├── OrcamentoTable.razor    # Table display
│   │   │   ├── OrcamentoFilters.razor  # Filter controls
│   │   │   └── LoginForm.razor         # Login form
│   │   ├── Services/
│   │   │   ├── AuthService.cs          # Frontend auth calls
│   │   │   ├── OrcamentoService.cs     # Frontend budget calls
│   │   │   └── CustomAuthenticationStateProvider.cs  # JWT auth state
│   │   ├── App.razor                   # Root component
│   │   ├── Program.cs                  # Startup
│   │   └── Frontend.csproj
│   │
│   └── Shared.csproj
│
├── ERP.Staccato.Web.sln                # Solution file
├── README.md                           # General info
├── IMPLEMENTATION_GUIDE.md             # This file
└── COMPARISON.md                       # TypeScript vs .NET comparison

```

## Key Architecture Decisions

### 1. Backend: ASP.NET Core REST API

- **Why REST instead of gRPC?** Simpler for Blazor WASM clients, better browser support
- **Why not SignalR?** POC focus on core functionality; can be added later
- **Authentication**: JWT tokens (same as TypeScript version)

### 2. Frontend: Blazor WebAssembly

- **Why Blazor WASM?** C# code in browser, type-safe, interops with .NET ecosystem
- **Why not Blazor Server?** No server affinity needed, works offline-first
- **State Management**: Cascading parameters + services (similar to React hooks)

### 3. Database Access

- **Why EF Core?** Industry standard, LINQ queries are type-safe
- **Why FromSqlRaw for Orcamentos?** Complex filtering requires dynamic SQL (same as TypeScript)
- **Database-First approach**: Works with existing C++ schema without migrations

### 4. Shared Types

- **Separate Shared project**: Compiled to assembly, referenced by both Backend and Frontend
- **No code generation**: All types manually defined (unlike Prisma/tRPC)
- **API Responses**: Standardized `ApiResponse<T>` wrapper

## Building and Running

### Prerequisites

```text
- .NET 9 SDK
- Visual Studio 2022 / VS Code + C# extensions
- MariaDB/MySQL connection (same database as C++ app)
```

### Backend Setup

```bash
cd src/Backend

# Restore dependencies
dotnet restore

# Update database connection in appsettings.json
# Server=localhost;Port=3306;Database=staccato;User=root;Password=;

# Run development server
dotnet run
# Listens on: https://localhost:7001
```

### Frontend Setup

```bash
cd src/Frontend

# Restore dependencies
dotnet restore

# Run development server with hot reload
dotnet run
# Listens on: https://localhost:7002
```

### Solution File (Visual Studio)

```bash
# Open solution
dotnet sln list

# Build all projects
dotnet build

# Run tests (when added)
dotnet test
```

## API Endpoints

### Authentication

```text
POST   /api/auth/login          # Login with credentials
GET    /api/auth/me             # Get current user (requires token)
```

### Orcamentos (Budgets)

```text
POST   /api/orcamento/list                  # List with filters (requires token)
GET    /api/orcamento/lojas                # Get stores dropdown (requires token)
GET    /api/orcamento/vendedores?idLoja=N # Get vendors dropdown (requires token)
GET    /api/orcamento/fornecedores        # Get suppliers dropdown (requires token)
```

## Frontend Components (To Be Implemented)

### Pages

- **Login.razor** - Authentication page
- **Dashboard.razor** - Main dashboard with navigation
- **Orcamentos.razor** - Budget list with filters (main page)

### Components

- **OrcamentoTable.razor** - Virtualized table display
- **OrcamentoFilters.razor** - Filter controls (lojas, vendedores, etc.)
- **LoginForm.razor** - Login input form
- **AuthorizeView** - Role-based authorization wrapper

## Comparison with TypeScript Version

| Feature                | TypeScript            | .NET                            | Notes                                 |
| ---------------------- | --------------------- | ------------------------------- | ------------------------------------- |
| **Backend Framework**  | NestJS                | ASP.NET Core                    | Both REST APIs with similar patterns  |
| **Frontend Framework** | React                 | Blazor WASM                     | Both SPA, similar component models    |
| **ORM**                | Prisma (schema-first) | EF Core (code-first)            | Different approaches, similar results |
| **API Communication**  | tRPC (typed RPC)      | REST (typed DTOs)               | tRPC is more type-safe end-to-end     |
| **State Management**   | Zustand (external)    | Cascading Parameters (built-in) | .NET is more integrated               |
| **Auth Provider**      | Custom + localStorage | AuthenticationStateProvider     | .NET has built-in auth abstractions   |
| **Database Queries**   | Raw SQL with Prisma   | Raw SQL with EF Core            | Both use raw SQL for complex queries  |
| **Bundle Size**        | ~100KB                | ~300KB (initial)                | WASM startup is heavier               |
| **Performance**        | Good                  | Good (faster rendering)         | .NET WASM improves each release       |

## Development Workflow

### 1. Creating a New API Endpoint

```csharp
// 1. Add DTO to Shared/Models/
public class MyDto { ... }

// 2. Add method to Service (Backend/Services/)
public async Task<MyDto> GetMyDataAsync() { ... }

// 3. Add Controller method (Backend/Controllers/)
[HttpGet("mydata")]
public async Task<ApiResponse<MyDto>> GetMyData() { ... }

// 4. Add Service method in Frontend (Frontend/Services/)
public async Task<MyDto> GetMyDataAsync() { ... }

// 5. Use in Component (Frontend/Pages/)
@inject OrcamentoService Service
@await Service.GetMyDataAsync()
```

### 2. Adding Filter Logic

1. Update `OrcamentoFilters` in Shared
2. Update SQL building in `OrcamentoService.ListAsync()`
3. Update filtering component binding
4. Test end-to-end

### 3. Debugging

```csharp
// Backend logging
_logger.LogInformation($"[Orcamento.List] Filters: {JsonSerializer.Serialize(filters)}");

// Frontend logging
Console.WriteLine("State changed");

// Network inspection: Browser DevTools → Network tab
// JWT inspection: jwt.io
```

## Next Steps (Priority Order)

### Phase 1: Core Functionality (1-2 weeks)

- [ ] Implement Blazor pages and components
- [ ] Wire up authentication flow
- [ ] Test API endpoints with Postman/curl
- [ ] Implement orcamento list with basic filters

### Phase 2: Feature Parity (1 week)

- [ ] Add all filters (status, date, vendor, etc.)
- [ ] Implement virtualized table (if needed)
- [ ] Add loading states and error handling
- [ ] Dark theme CSS

### Phase 3: Advanced Features (ongoing)

- [ ] Real-time updates (SignalR)
- [ ] Bulk operations
- [ ] Export to Excel
- [ ] Advanced analytics

### Phase 4: Production Ready (2-3 weeks)

- [ ] Unit tests (xUnit)
- [ ] Integration tests
- [ ] Performance optimization
- [ ] Security audit (CORS, CSRF, etc.)

## Useful Resources

- [ASP.NET Core Docs](https://learn.microsoft.com/en-us/aspnet/core/)
- [Blazor Docs](https://learn.microsoft.com/en-us/aspnet/core/blazor/)
- [Entity Framework Core](https://learn.microsoft.com/en-us/ef/core/)
- [JWT in .NET](https://learn.microsoft.com/en-us/dotnet/api/system.identitymodel.tokens.jwt)
- [Blazor Authentication](https://learn.microsoft.com/en-us/aspnet/core/blazor/security/)

## Troubleshooting

### "Connection refused" to backend

- Check backend is running: `https://localhost:7001/health`
- Check CORS policy in Program.cs matches frontend URL
- Check JWT configuration

### JWT token not working

- Verify token format: Bearer <token>
- Check JWT secret matches between appsettings.json and token generation
- Inspect token at jwt.io

### Blazor not loading

- Check frontend is running: `https://localhost:7002`
- Check console for WASM loading errors
- Verify Framework: net9.0 matches SDK version

## Contributing

When adding features:

1. Follow existing patterns (Services, Controllers, DTOs)
2. Add logging for debugging
3. Update this guide if architecture changes
4. Keep TypeScript and .NET versions in sync conceptually

## License

Same as parent ERP Staccato project
