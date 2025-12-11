# Web Migration Language Comparison - Brazilian Library Ecosystem

## Overview

This document compares TypeScript and C#/.NET for migrating the ERP Staccato C++ codebase to web, with focus on Brazilian-specific library support.

## Key Requirements

- **Type Safety**: Catch issues at compile time
- **NFe Integration**: Nota Fiscal Eletronica (critical for Brazilian compliance)
- **CNAB/Boleto**: Bank slip generation and CNAB file processing
- **PIX**: Instant payment system support
- **CPF/CNPJ**: Brazilian document validation

---

## Current Architecture: ACBr via TCP/IP

**Important**: The current system uses ACBr in TCP/IP server mode. This means:

- ACBr runs as a separate service exposing a TCP interface
- The ERP application is a TCP client sending commands
- All fiscal complexity (XML, certificates, SEFAZ) is handled by ACBr
- **Language choice for the web app does NOT affect NFe integration**

```
┌─────────────────┐         TCP          ┌─────────────────┐
│   Web App       │◄───────────────────► │   ACBr Server   │
│  (Any Language) │   Commands/Response  │   (Delphi)      │
└─────────────────┘                      └────────┬────────┘
                                                  │
                                                  ▼
                                         ┌─────────────────┐
                                         │     SEFAZ       │
                                         └─────────────────┘
```

### TCP Communication Example

Both languages handle ACBr TCP communication trivially:

```typescript
// TypeScript - TCP to ACBr
import * as net from 'net';

const socket = new net.Socket();
socket.connect(3434, 'localhost');
socket.write('NFE.StatusServico\r\n');
socket.on('data', (data) => console.log(data.toString()));
```

```csharp
// C# - Same simplicity
using var client = new TcpClient("localhost", 3434);
var stream = client.GetStream();
await stream.WriteAsync(Encoding.UTF8.GetBytes("NFE.StatusServico\r\n"));
```

---

## Impact on Language Choice

| Concern | Handled By | Impact on Language Choice |
|---------|------------|---------------------------|
| NFe/NFCe emission | ACBr (TCP) | **None** |
| Certificate signing | ACBr (TCP) | **None** |
| SEFAZ communication | ACBr (TCP) | **None** |
| XML generation | ACBr (TCP) | **None** |
| Tax Reform 2025 | ACBr (update server) | **None** |
| CNAB/Boleto | See below | Minor |
| PIX | Bank APIs | **None** |
| CPF/CNPJ validation | Simple logic | **None** |

**Conclusion**: Since ACBr handles all fiscal complexity via TCP, the language choice should be based on **web development merits**, not Brazilian fiscal library availability.

---

## Library Comparison (For Reference)

### NFe (Nota Fiscal Eletronica)

> **Note**: This section is for reference only. With ACBr TCP architecture, these libraries are NOT needed.

#### TypeScript/Node.js

| Library | Description | Status |
|---------|-------------|--------|
| [NFeWizard-io](https://github.com/nfewizard-org/nfewizard-io) | Full SEFAZ integration | Active |
| [NFe.io](https://nfe.io/docs/desenvolvedores/bibliotecas/node-js/) | SaaS API | Paid service |

#### C# / .NET

| Library | Description | Status |
|---------|-------------|--------|
| [DFe.NET](https://github.com/ZeusAutomacao/DFe.NET) | Complete NFe/NFCe support | Active |
| [ACBr.Net.NFe](https://github.com/cecon/ACBr.Net.NFe) | .NET port of ACBr | Active |

#### Winner: N/A - ACBr handles this via TCP

---

### CNAB / Boleto Bancario

#### TypeScript/Node.js

| Library | Description | Status |
|---------|-------------|--------|
| [@banco-br/nodejs-cnab](https://github.com/banco-br/nodejs-cnab) | CNAB 240/400 support | Inactive (12+ months) |
| [boleto.js](https://www.guilhermearaujo.dev/boleto.js/) | SVG barcode generation | Limited scope |

#### C# / .NET

| Library | Description | Status |
|---------|-------------|--------|
| [Boleto.Net](https://github.com/BoletoNet/boletonet) | Complete boleto/CNAB solution | Active, 468k+ downloads |

#### Assessment

C# has better CNAB libraries, but this is a **minor consideration** because:
1. CNAB file generation is straightforward text/fixed-width format
2. Can be implemented directly without heavy libraries
3. Or use a simple microservice if needed

---

### PIX (Instant Payments)

#### TypeScript/Node.js

| Library | Description | Status |
|---------|-------------|--------|
| [pix-utils](https://www.npmjs.com/package/pix-utils) | Parse, generate, validate PIX | Active |
| [gpix](https://github.com/hiagodotme/gpix) | Static/dynamic BR codes | Active |

#### C# / .NET

| Library | Description | Status |
|---------|-------------|--------|
| [pix-payload-generator.net](https://github.com/alexandresanlim/pix-payload-generator.net) | Static QR code generation | Active |

#### Winner: Tie

Both adequate. Full PIX API requires bank/PSP SDKs regardless.

---

### CPF/CNPJ Validation

#### TypeScript/Node.js

| Library | Description | Status |
|---------|-------------|--------|
| [validation-br](https://www.npmjs.com/package/validation-br) | CPF, CNPJ, Titulo Eleitoral, PIS, CNH | Active |
| [cpf-cnpj-validator](https://www.npmjs.com/package/cpf-cnpj-validator) | CPF/CNPJ with Joi integration | Active |

**Note**: Already supports alphanumeric CNPJ (effective 2026)

#### C# / .NET

- Built into most Brazilian libraries
- Standard validation available

#### Winner: TypeScript

Better ecosystem, prepared for 2026 changes.

---

## Summary Comparison (Revised)

| Category | TypeScript/Node.js | C# / .NET | Winner |
|----------|-------------------|-----------|--------|
| **NFe/NFCe** | N/A (ACBr TCP) | N/A (ACBr TCP) | **Tie** |
| **CNAB/Boleto** | Weak | Good | C# (minor) |
| **PIX** | Good | Good | Tie |
| **CPF/CNPJ** | Excellent | Good | TypeScript |
| **Type Safety** | Excellent | Excellent | Tie |
| **Web Dev Ecosystem** | Excellent | Good | **TypeScript** |
| **Frontend Integration** | Native | Requires separate | **TypeScript** |
| **Full-Stack Type Sharing** | Yes | Limited | **TypeScript** |
| **Hiring (Web Dev)** | Easier | Moderate | **TypeScript** |
| **Learning Curve (from C++)** | Medium | Lower | C# (minor) |

---

## Recommendation: TypeScript Full-Stack

Given that ACBr handles fiscal operations via TCP, **TypeScript is the recommended choice**.

### Recommended Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Frontend                             │
│              React + TypeScript                         │
│         (Shared types with backend)                     │
└─────────────────────┬───────────────────────────────────┘
                      │ HTTP/WebSocket
                      ▼
┌─────────────────────────────────────────────────────────┐
│                    Backend                              │
│           Node.js + TypeScript                          │
│        (NestJS, Fastify, or Express)                    │
├─────────────────────┬───────────────────────────────────┤
│                     │                                   │
│    ┌────────────────┼────────────────┐                  │
│    ▼                ▼                ▼                  │
│ ┌──────┐      ┌──────────┐     ┌──────────┐            │
│ │ MySQL│      │ ACBr TCP │     │ Bank APIs│            │
│ └──────┘      └──────────┘     │ (PIX)    │            │
│                                └──────────┘            │
└─────────────────────────────────────────────────────────┘
```

### Technology Stack

| Layer | Technology | Rationale |
|-------|------------|-----------|
| **Frontend** | React + TypeScript | Largest ecosystem, type safety |
| **Backend** | NestJS + TypeScript | Structured, enterprise-ready, full TS |
| **Database** | MySQL/MariaDB (keep) or PostgreSQL | Minimize migration risk |
| **ORM** | Prisma or TypeORM | Type-safe database access |
| **NFe/Fiscal** | ACBr via TCP | Keep existing infrastructure |
| **Validation** | validation-br, zod | Brazilian docs + schema validation |

### Why TypeScript Wins

1. **Full-stack type sharing** - Define types once, use on frontend and backend
2. **Single language** - Entire web team uses one language
3. **Best web ecosystem** - npm has packages for everything
4. **Modern tooling** - Hot reload, excellent IDE support, fast iteration
5. **Hiring** - Large pool of web developers in Brazil
6. **ACBr abstraction** - Fiscal complexity is already handled externally

### CNAB Consideration

For CNAB/Boleto, two options:

**Option A**: Implement directly in TypeScript
- CNAB is fixed-width text format, not complex
- Can port existing logic from C++ codebase

**Option B**: Small C# microservice (if needed)
- Only for CNAB generation
- Communicate via HTTP API
- Leverage Boleto.Net maturity

---

## Alternative: C# / .NET

Still viable if:
- Team has strong C# experience
- Prefer similarity to C++ syntax
- Want Blazor for frontend (smaller ecosystem though)

```
Backend:  ASP.NET Core
Frontend: Blazor WebAssembly or React + TypeScript
Database: Keep MySQL/MariaDB or migrate to PostgreSQL
ACBr:     TCP communication (same as TypeScript)
```

---

## Migration Strategy

1. **Migrate first, restructure second** - Create faithful port before improving architecture
2. **Keep ACBr TCP** - No need to change fiscal infrastructure
3. **Database schema** - Fix fundamental issues during migration
4. **Module-by-module** - Migrate incrementally, not big-bang
5. **Parallel operation** - Run both systems until web version is validated

---

## References

### TypeScript/Node.js
- NestJS: https://nestjs.com/
- Prisma ORM: https://www.prisma.io/
- validation-br: https://www.npmjs.com/package/validation-br
- pix-utils: https://www.npmjs.com/package/pix-utils

### C# / .NET (for reference)
- DFe.NET: https://github.com/ZeusAutomacao/DFe.NET
- Boleto.Net: https://github.com/BoletoNet/boletonet

### ACBr
- ACBr Project: https://projetoacbr.com.br/
- ACBrMonitor TCP: https://projetoacbr.com.br/acbrmonitor/
