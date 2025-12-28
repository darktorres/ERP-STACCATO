# ERP Staccato Web

Web migration of the C++ ERP Staccato application.

## Stack

- **Backend**: NestJS + Fastify + Prisma + tRPC
- **Frontend**: React + TypeScript + TailwindCSS + tRPC
- **Database**: MySQL (existing)

## Project Structure

```text
web/
├── packages/
│   ├── backend/          # NestJS backend
│   │   ├── prisma/       # Database schema
│   │   └── src/
│   │       ├── modules/  # Feature modules (auth, user, etc.)
│   │       ├── prisma/   # Prisma service
│   │       └── trpc/     # tRPC routers
│   ├── frontend/         # React frontend
│   │   └── src/
│   │       ├── components/
│   │       ├── hooks/
│   │       ├── lib/
│   │       ├── pages/
│   │       └── stores/
│   └── shared/           # Shared types and schemas
│       └── src/
│           ├── schemas/  # Zod validation schemas
│           └── types/    # TypeScript types
└── package.json          # Root workspace config
```

## Getting Started

### Prerequisites

- Node.js 20+
- npm 10+
- Access to MySQL database (staccato)

### Installation

```bash
cd web

# Install all dependencies
npm install

# Copy environment file
cp packages/backend/.env.example packages/backend/.env

# Edit .env with your database credentials
```

### Configure Database

Edit `packages/backend/.env`:

```env
DATABASE_URL="mysql://USER:PASSWORD@HOST:3306/staccato"
JWT_SECRET="your-secret-key"
```

### Generate Prisma Client

```bash
npm run db:generate
```

### Development

```bash
# Run both backend and frontend
npm run dev

# Or run separately:
npm run dev:backend  # http://localhost:3001
npm run dev:frontend # http://localhost:5173
```

## Migration Status

### Completed

- [x] Login Dialog (basic)

### Pending

- [ ] Main Window
- [ ] Compras module
- [ ] Estoque module
- [ ] Financeiro module
- [ ] Logística module
- [ ] NFe module
- [ ] Reports

## Architecture Notes

### Authentication

The login system mirrors the C++ implementation:

1. User enters credentials
2. Backend queries MySQL using `SHA_PASSWORD()` for password verification
3. JWT token is issued on success
4. Frontend stores token and includes in subsequent requests

### Type Safety

tRPC provides end-to-end type safety:

```typescript
// Backend defines procedure
auth: this.trpc.router({
  login: this.trpc.procedure
    .input(loginSchema)
    .mutation(async ({ input }) => { ... })
})

// Frontend gets full type inference
const mutation = trpc.auth.login.useMutation();
mutation.mutate({ user: '...', password: '...' }); // Fully typed!
```

### Database

Using Prisma ORM with the existing MySQL schema. The Prisma schema in `packages/backend/prisma/schema.prisma` maps to existing tables.

**Important**: We're connecting to the existing database, not creating a new one. Be careful with migrations.
