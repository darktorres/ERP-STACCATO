# Symfony Web Implementation Plan

## Overview
Implementing a Symfony 8.0 **web application** for the ERP Staccato system based on the C++ Qt application.

**Scope:**
1. **Login Page** - Replicate LoginDialog.cpp behavior with login form
2. **Quotation Page** - Replicate WidgetOrcamento.cpp with filtering and listing

**Technology Stack:**
- Symfony 8.0 (backend framework)
- Twig (templating engine)
- Bootstrap 5 (responsive UI)
- JavaScript/HTMX (dynamic interactions)

**NOT Included:** Quote creation, editing, deletion, item management, or any write operations.

## Current Status

### Completed ✅
1. **Composer Dependencies** - All Symfony 8.0 packages installed successfully
   - Framework, Security, ORM, Serializer bundles configured
   - JWT (lcobucci/jwt ^4.2) installed for token management
   - Doctrine ORM 3.5 with MySQL support

2. **Doctrine Entities Created**
   - `src/Entity/Loja.php` - Store/Location entity
   - `src/Entity/Usuario.php` - User entity with role methods
   - `src/Entity/Maintenance.php` - Maintenance mode flag
   - `src/Entity/Orcamento.php` - Quotation entity with validity calculation

### Pending Tasks

## Phase 1: Authentication System (Priority: HIGH)

### 1.1 Database Configuration & Migrations
**Files to create/modify:**
- `.env` - Update DATABASE_URL with MySQL connection details
- `config/packages/doctrine.yaml` - Already created by Flex, may need adjustments
- Create migration files for schema if needed

**Notes:**
- System uses existing MySQL database from C++ application
- No schema creation needed - database already exists
- Configure connection string: `mysql://user:password@localhost:3306/staccato_db`

### 1.2 JWT Authentication Service
**File:** `src/Service/AuthService.php`

**Requirements:**
- Use MySQL `SHA_PASSWORD()` function for password verification
- Match C++ password validation: `WHERE password = SHA_PASSWORD(?)`
- Generate JWT tokens using lcobucci/jwt
- Support one-time password (`senhaUsoUnico`) for authorization mode
- Check maintenance mode before allowing login
- Log authentication attempts

**Key Logic:**
```php
// Query structure matching C++
SELECT idLoja, idUsuario, nome, tipo FROM usuario
WHERE UPPER(user) = UPPER(?)
AND password = SHA_PASSWORD(?)
AND desativado = FALSE
```

### 1.3 User Provider
**File:** `src/Security/UserProvider.php`

**Requirements:**
- Implement Symfony's UserProviderInterface
- Load user from database using Usuario entity
- Support role/type system from C++

**User Type Hierarchy (from C++):**
- `ADMINISTRADOR` - Admin access
- `DIRETOR` - Director access
- `ADMINISTRATIVO` - Administrative access
- `GERENTE LOJA` - Store Manager
- `GERENTE DEPARTAMENTO` - Department Manager
- `GERENTE FINANCEIRO` - Finance Manager
- `VENDEDOR ESPECIAL` - Special Seller
- `VENDEDOR` - Regular Seller
- `ASSISTENTE ADMINISTRATIVO` - Admin Assistant
- `OPERACIONAL` - Operational (blocked from API)

### 1.4 JWT Authenticator
**File:** `src/Security/JwtAuthenticator.php`

**Requirements:**
- Implement Symfony's AuthenticatorInterface
- Extract JWT token from `Authorization: Bearer <token>` header
- Validate and decode JWT token
- Load user from token claims
- Return authenticated token with user

### 1.5 Security Configuration
**File:** `config/packages/security.yaml`

**Requirements:**
- Configure JWT authenticator for API routes
- Set up firewall for `/api` endpoints
- Configure access control by user type
- Support stateless authentication (no sessions)

**Route Protection:**
- `/api/auth/login` - Public
- `/api/auth/logout` - Requires JWT_AUTHENTICATED_FULLY
- `/api/auth/me` - Requires JWT_AUTHENTICATED_FULLY
- `/api/orcamentos/*` - Requires JWT_AUTHENTICATED_FULLY

## Phase 2: Login Page (Web UI)

### 2.1 Login Controller
**File:** `src/Controller/AuthController.php`

**Routes:**
- `GET /` - Display login page
- `POST /login` - Process login form
- `GET /logout` - Logout and redirect to login

### 2.2 Login Form Template
**File:** `templates/auth/login.html.twig`

**Features (from C++ LoginDialog):**
- Username input field
- Password input field
- Remember password checkbox
- "Login" button
- Error messages display
- Simple Portuguese UI

### 2.3 Session/Cookie Management
- Store user session after login
- Redirect to quotations page on success
- Show error messages on login failure

## Phase 3: Quotations Page (Web UI)

### 3.1 Quotation Controller
**File:** `src/Controller/QuotationController.php`

**Routes:**
- `GET /orcamentos` - Display quotation list with filters
- `GET /orcamentos/data` - Return filtered data (AJAX)

### 3.2 Quotation Page Template
**File:** `templates/quotation/list.html.twig`

**Layout (matching WidgetOrcamento):**
- Left sidebar: Filters
- Main content: Quotation table

**Filter Options:**
- Store dropdown (idLoja)
- Month picker (mesAno)
- Seller dropdown (idVendedor)
- Supplier dropdown (fornecedor)
- Status checkboxes (ATIVO, CANCELADO, EXPIRADO, FECHADO, PERDIDO, REPLICADO)
- Semáforo radio buttons (QUENTE, MORNO, FRIO)
- "Only own quotes" checkbox
- Text search input

**Table Columns:**
- ID (idOrcamento)
- Vendor (vendedor)
- Client (cliente)
- Professional (profissional)
- Status
- Days Remaining (diasRestantes)
- Date (data)
- Total (total)
- Notes (observacao)

### 3.3 AJAX Filtering
- Form submission triggers AJAX call
- Updates table without page reload
- Dynamic dropdown population
- Real-time search

## Phase 4: OLD - Authentication API Endpoints (DEPRECATED - Now Web-Based)

These are now web pages instead of APIs.

#### OLD: POST /api/auth/login
**Request:**
```json
{
  "user": "username",
  "password": "password"
}
```

**Response (Success):**
```json
{
  "success": true,
  "token": "eyJ0eXAiOiJKV1QiLCJhbGc...",
  "user": {
    "idUsuario": 1,
    "idLoja": 1,
    "user": "username",
    "nome": "Full Name",
    "tipo": "GERENTE LOJA",
    "loja": {
      "idLoja": 1,
      "descricao": "Store 1",
      "nomeFantasia": "Store Name"
    }
  }
}
```

**Response (Failure):**
```json
{
  "success": false,
  "error": "Login inválido!" (or "Sistema em manutenção!" or "Operacional bloqueado!")
}
```

**Logic:**
1. Validate input (user, password required)
2. Check maintenance mode
3. Query usuario with SHA_PASSWORD verification
4. Block OPERACIONAL user type
5. Generate JWT token with claims: idUsuario, idLoja, tipo
6. Return token and user info

#### GET /api/auth/me
**Response:**
```json
{
  "idUsuario": 1,
  "idLoja": 1,
  "user": "username",
  "nome": "Full Name",
  "tipo": "GERENTE LOJA",
  "loja": {
    "idLoja": 1,
    "descricao": "Store 1",
    "nomeFantasia": "Store Name"
  }
}
```

**Logic:**
1. Get authenticated user from JWT
2. Load related loja relationship
3. Return user info

#### POST /api/auth/logout
**Response:**
```json
{
  "message": "Logged out successfully"
}
```

**Logic:**
- Stateless, just return success message
- Client discards token on their side

## Phase 3: Orcamento (Quotation) Listing System

**Scope:** Read-only listing and filtering based on WidgetOrcamento, NOT quote CRUD operations.

### 3.1 OrcamentoView Entity
**File:** `src/Entity/OrcamentoView.php`

**Purpose:**
- Read-only Doctrine entity mapping to database view `view_orcamento`
- Displays computed/joined data for API responses
- Used only for SELECT queries

**Fields (from C++ WidgetOrcamento display):**
- `idOrcamento` - Quote ID
- `idLoja` - Store ID
- `idUsuario` - Seller user ID
- `idUsuarioConsultor` - Consultant user ID
- `status` - Quote status (ATIVO, CANCELADO, EXPIRADO, FECHADO, PERDIDO, REPLICADO)
- `diasRestantes` - Remaining validity days (computed)
- `vendedor` - Seller name (from join)
- `consultor` - Consultant name (from join)
- `cliente` - Client name (from join)
- `profissional` - Professional name (from join)
- `tel`, `telCel`, `telProf` - Phone numbers
- `data`, `data2` - Dates (data2 is YYYY-MM for filtering)
- `total` - Quote total
- `dataFollowup`, `dataProxFollowup` - Follow-up dates
- `observacao` - Notes
- `semaforo` - Follow-up color (1=QUENTE, 2=MORNO, 3=FRIO)
- `fornecedores` - Supplier list (comma-separated)

### 3.2 OrcamentoController (READ-ONLY)
**File:** `src/Controller/OrcamentoController.php`

**Endpoints:**

#### GET /api/orcamentos/list
**Query Parameters (all optional):**
```
mesAno=YYYY-MM
idLoja=1
idVendedor=1
fornecedor=Supplier Name
statuses[]=ATIVO&statuses[]=CANCELADO
semaforo=1
apenasPropriosOrcamentos=true
search=text
```

**Response:**
```json
{
  "success": true,
  "data": [
    {
      "idOrcamento": "ORC-001",
      "idLoja": 1,
      "idUsuario": 1,
      "idUsuarioConsultor": 2,
      "status": "ATIVO",
      "diasRestantes": "5",
      "vendedor": "John Doe",
      "consultor": "Jane Smith",
      "cliente": "Client Name",
      "profissional": "Pro Name",
      "tel": "1234567890",
      "telCel": "98765432100",
      "telProf": "1234567890",
      "data": "2024-12-13T10:30:00Z",
      "data2": "2024-12",
      "total": "5000.00",
      "dataFollowup": "2024-12-10T14:00:00Z",
      "dataProxFollowup": "2024-12-20T14:00:00Z",
      "observacao": "Some notes",
      "semaforo": 1,
      "fornecedores": "Supplier 1, Supplier 2"
    }
  ]
}
```

**Filtering Logic (from C++):**
1. **Role-based filtering:**
   - GERENTE LOJA/DEPARTAMENTO: Only own store (`idLoja = user.idLoja`)
   - VENDEDOR/VENDEDOR ESPECIAL: Only own quotes if `apenasPropriosOrcamentos=true`
   - Others: Can filter by store or see all based on params

2. **Available filters:**
   - Month: `data2` field matches `YYYY-MM`
   - Seller: `idUsuario` or `idUsuarioConsultor` matches
   - Supplier: FIND_IN_SET search in `fornecedores`
   - Status: Checkboxes for ATIVO, CANCELADO, EXPIRADO, FECHADO, PERDIDO, REPLICADO
   - Semáforo: Filter by follow-up color
   - Own quotes: Name comparison with `vendedor` or `consultor` fields
   - Text search: By idOrcamento, vendedor, cliente, profissional

3. **Ordering:** By date DESC

#### GET /api/orcamentos/lojas
**Response:**
```json
{
  "success": true,
  "data": [
    {
      "idLoja": 1,
      "descricao": "Main Store",
      "nomeFantasia": "Store 1"
    }
  ]
}
```

**Logic:**
- Return all active stores (desativado = false)
- Order by idLoja

#### GET /api/orcamentos/vendedores
**Query Parameters:**
```
idLoja=1 (optional)
```

**Response:**
```json
{
  "success": true,
  "data": [
    {
      "idUsuario": 1,
      "nome": "John Doe"
    }
  ]
}
```

**Logic:**
- Return VENDEDOR and VENDEDOR ESPECIAL users
- Filter by store if provided
- Order by name

#### GET /api/orcamentos/fornecedores
**Response:**
```json
{
  "success": true,
  "data": [
    {
      "razaoSocial": "Supplier Name 1"
    },
    {
      "razaoSocial": "Supplier Name 2"
    }
  ]
}
```

**Logic:**
- Extract unique suppliers from all orcamentos' `fornecedores` field
- Parse comma-separated values
- Sort alphabetically
- Deduplicate

### 3.3 Quote Status Values (from C++)
- `ATIVO` - Active/Valid
- `EXPIRADO` - Expired (diasRestantes < 0)
- `CANCELADO` - Cancelled
- `FECHADO` - Closed (converted to sale)
- `PERDIDO` - Lost
- `REPLICADO` - Replicated from expired quote

## Phase 4: API Routes

### 4.1 Routes Configuration
**File:** `config/routes/api.yaml`

```yaml
# Authentication routes
api_auth_login:
  path: /api/auth/login
  methods: POST
  controller: App\Controller\AuthController::login

api_auth_me:
  path: /api/auth/me
  methods: GET
  controller: App\Controller\AuthController::me

api_auth_logout:
  path: /api/auth/logout
  methods: POST
  controller: App\Controller\AuthController::logout

# Orcamento routes
api_orcamentos_list:
  path: /api/orcamentos/list
  methods: GET
  controller: App\Controller\OrcamentoController::list

api_orcamentos_lojas:
  path: /api/orcamentos/lojas
  methods: GET
  controller: App\Controller\OrcamentoController::lojas

api_orcamentos_vendedores:
  path: /api/orcamentos/vendedores
  methods: GET
  controller: App\Controller\OrcamentoController::vendedores

api_orcamentos_fornecedores:
  path: /api/orcamentos/fornecedores
  methods: GET
  controller: App\Controller\OrcamentoController::fornecedores
```

## Phase 5: Request/Response DTOs & Validation

### 5.1 DTOs (Data Transfer Objects)
**Files:**
- `src/DTO/LoginRequest.php` - Login request validation
- `src/DTO/UserResponse.php` - User info response
- `src/DTO/OrcamentoResponse.php` - Quotation response
- `src/DTO/FilterResponse.php` - Filter options response

**Validation:**
- Use Symfony Validator component
- Validate input types and constraints
- Return validation errors in API response

## Phase 6: Error Handling & Response Formatting

### 6.1 Exception Handler
**File:** `src/EventListener/ExceptionListener.php`

**Standard Error Response:**
```json
{
  "success": false,
  "error": "Error message",
  "code": "ERROR_CODE"
}
```

**HTTP Status Codes:**
- 200 - Success
- 400 - Bad request / validation error
- 401 - Unauthorized (invalid credentials, missing token)
- 403 - Forbidden (insufficient permissions)
- 404 - Not found
- 500 - Server error
- 503 - Service unavailable (maintenance mode)

## Implementation Order

1. **Database Configuration** (.env, doctrine.yaml)
2. **Entities** (Loja, Usuario, Maintenance, Orcamento - already done)
3. **AuthService** (password hashing with MySQL SHA_PASSWORD, JWT generation)
4. **UserProvider & JwtAuthenticator** (security layer for JWT)
5. **Security Configuration** (firewall, access control for API)
6. **AuthController** (login, me, logout endpoints)
7. **OrcamentoView Entity** (read-only view mapping)
8. **OrcamentoController** (list with filters, dropdown endpoints)
9. **API Routes** (route definitions)
10. **DTOs & Validation** (request/response handling)
11. **Exception Handling** (error responses)
12. **Testing** (API endpoint tests)

## Key Implementation Details (from C++ codebase)

### Authentication (LoginDialog)
1. **Password Hashing:** Uses MySQL `SHA_PASSWORD()` function, not bcrypt
2. **User Types:** String constants in database (VENDEDOR, GERENTE LOJA, etc.)
3. **Maintenance Mode:** Simple boolean flag in `maintenance` table checked before allowing login
4. **One-Time Passwords:** Separate field (`senhaUsoUnico`) for authorization mode
5. **Minimum Freight Value:** User-specific field for freight authorization
6. **Login Blocking:** OPERACIONAL user type is blocked from API access

### Quotation Listing (WidgetOrcamento)
1. **Quote Filtering:** Complex role-based and user-specific filtering logic:
   - GERENTE LOJA/DEPARTAMENTO: Restricted to their store only
   - VENDEDOR/VENDEDOR ESPECIAL: Can filter own quotes or all (based on param)
   - Admin/Administrative: Can filter by month and see all stores
   - Others: See based on role permissions

2. **Filter Parameters:**
   - Month (data2 field in YYYY-MM format)
   - Store (idLoja)
   - Seller (idUsuario or idUsuarioConsultor)
   - Supplier (FIND_IN_SET in fornecedores field)
   - Status checkboxes (ATIVO, CANCELADO, EXPIRADO, FECHADO, PERDIDO, REPLICADO)
   - Semáforo/Follow-up level (1=QUENTE, 2=MORNO, 3=FRIO)
   - Own quotes filter (name-based comparison)
   - Text search (ID, vendor, client, professional)

3. **Quote Validity:** Days-based expiration calculated on-the-fly from creation date + validity days

## Important Notes

- Database already exists from C++ application - no schema migration needed
- All enum-like values (user types, statuses) are VARCHAR in database
- The view `view_orcamento` is used for API responses instead of joining tables
- Authentication is stateless (JWT tokens, no sessions)
- All API responses follow the `{ success: boolean, data?: any, error?: string }` pattern
- Role checks are done server-side on every request
- Quote expiration is calculated on-the-fly, not stored

## Files to Create Summary

```
src/
  Entity/
    Loja.php ✅
    Usuario.php ✅
    Maintenance.php ✅
    Orcamento.php ✅
    OrcamentoView.php (TODO)
  Service/
    AuthService.php (TODO)
  Security/
    UserProvider.php (TODO)
    JwtAuthenticator.php (TODO)
  Controller/
    AuthController.php (TODO)
    OrcamentoController.php (TODO)
  EventListener/
    ExceptionListener.php (TODO)
  DTO/
    LoginRequest.php (TODO)
    UserResponse.php (TODO)
    OrcamentoResponse.php (TODO)
config/
  packages/
    security.yaml (TODO - needs update)
  routes/
    api.yaml (TODO)
.env (TODO - update DATABASE_URL)
```
