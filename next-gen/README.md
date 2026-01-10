# ERP Staccato - Next Generation

> Laravel implementation of the next-generation ERP system.

## Overview

This folder contains the new Laravel-based ERP implementation, migrating from the legacy Qt/C++ application.

## Structure

```
next-gen/
├── app/
│   ├── Contracts/           # Interfaces
│   ├── Enums/               # PHP 8.1+ Enums
│   ├── Events/              # Domain events
│   ├── Exceptions/          # Custom exceptions
│   ├── Http/
│   │   ├── Controllers/
│   │   ├── Middleware/
│   │   └── Requests/
│   ├── Models/              # Eloquent models
│   ├── Policies/            # Authorization policies
│   ├── Providers/
│   ├── Services/            # Business logic
│   └── Traits/              # Reusable traits
├── config/
├── database/
│   ├── migrations/
│   └── seeders/
├── resources/
│   └── views/
├── routes/
├── tests/
│   ├── Feature/
│   └── Unit/
└── ...
```

## Documentation

All specifications and architecture documentation are located in:

```
.claude/next-gen/
├── 01-contexto/             # Business context and flows
├── 02-legado/               # Legacy system analysis
├── 03-requisitos/           # Requirements
├── 04-arquitetura/          # Architecture specs
│   ├── modulos/             # Module specifications
│   │   ├── cadastros.md
│   │   ├── compras.md
│   │   ├── estoque.md
│   │   ├── financeiro.md
│   │   ├── vendas.md
│   │   ├── nfe.md
│   │   ├── logistica.md
│   │   ├── notificacoes.md
│   │   └── relatorios.md
│   └── expansao-futura.md   # Future expansion roadmap
├── 05-execucao/             # Execution plans
└── 06-testes/               # Test specifications
```

## Getting Started

### Requirements

- PHP 8.2+
- PostgreSQL 15+
- Redis
- Composer
- Node.js 18+ (for frontend assets)

### Installation

```bash
cd next-gen
composer install
cp .env.example .env
php artisan key:generate
php artisan migrate
php artisan db:seed
npm install && npm run build
```

### Development

```bash
php artisan serve
npm run dev
```

### Testing

```bash
php artisan test
php artisan test --coverage
```

## Module Implementation Status

| Module | Status | Priority |
|--------|--------|----------|
| Cadastros | Not Started | 1 |
| Compras | Not Started | 2 |
| Estoque | Not Started | 3 |
| Financeiro | Not Started | 4 |
| Vendas | Not Started | 5 |
| NFe | Not Started | 6 |
| Logistica | Not Started | 7 |
| Notificacoes | Not Started | 8 |
| Relatorios | Not Started | 9 |

## Cross-Cutting Concerns

| Feature | Status |
|---------|--------|
| Audit Trail | Not Started |
| RBAC Permissions | Not Started |
| Idempotency | Not Started |
| Approval Workflows | Not Started |

## Tech Stack

- **Backend**: Laravel 11
- **Database**: PostgreSQL 15
- **Cache**: Redis
- **Queue**: Redis + Laravel Horizon
- **Search**: PostgreSQL FTS (or Meilisearch)
- **Real-time**: Laravel Echo + Pusher/Soketi
- **Frontend**: Livewire 3 + Alpine.js + Tailwind CSS
- **Reports**: Laravel Excel + DomPDF
- **Testing**: PHPUnit + Pest

## Related Documentation

- [Architecture Overview](../.claude/next-gen/04-arquitetura/01-arquitetura.md)
- [Module Specifications](../.claude/next-gen/04-arquitetura/modulos/_indice.md)
- [Future Expansion](../.claude/next-gen/04-arquitetura/expansao-futura.md)
