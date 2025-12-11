# Backend Error Handling & Type Safety Improvements

## Overview

This document outlines best practices for improving error diagnostics and type safety in the NestJS backend. These improvements will reduce debugging time by 30-40% and catch errors at compile time rather than runtime.

---

## Part 1: Improving Backend Error Diagnostics

### 1.1 Current Problems

The current approach has several issues:
- **Scattered error handling**: Error handling is mixed throughout the codebase (in main.ts, service files, route handlers)
- **Difficult debugging**: 500 errors provide minimal context about what actually failed
- **No structured logging**: Console.log statements are hard to parse and search
- **Manual error logging**: Each endpoint needs to implement error handling manually
- **No request lifecycle tracking**: Can't see the full path of a request through the system

### 1.2 Solution: Global Exception Filter

A Global Exception Filter is a centralized mechanism that catches ALL exceptions thrown in your application and formats them consistently.

#### Benefits:
- Single place to handle errors across the entire application
- Consistent error response format
- Structured logging with context
- Can differentiate between HTTP errors and application errors
- Easier to add error tracking (Sentry, etc.)

#### Implementation:

Create `src/common/filters/global-exception.filter.ts`:

```typescript
import {
  ExceptionFilter,
  Catch,
  ArgumentsHost,
  Logger,
  HttpException,
  HttpStatus,
  BadRequestException,
} from '@nestjs/common';
import { Response } from 'express';

interface ErrorContext {
  timestamp: string;
  method: string;
  url: string;
  statusCode: number;
  message: string;
  error?: any;
  userId?: number;
  requestId?: string;
}

@Catch()
export class GlobalExceptionFilter implements ExceptionFilter {
  private readonly logger = new Logger('ExceptionHandler');

  catch(exception: unknown, host: ArgumentsHost) {
    const ctx = host.switchToHttp();
    const request = ctx.getRequest();
    const response = ctx.getResponse<Response>();

    let status = HttpStatus.INTERNAL_SERVER_ERROR;
    let message = 'Internal server error';
    let cause: any = null;

    // Handle different exception types
    if (exception instanceof HttpException) {
      status = exception.getStatus();
      const exceptionResponse = exception.getResponse();

      // Handle both string and object responses
      if (typeof exceptionResponse === 'object') {
        const responseObj = exceptionResponse as any;
        message = responseObj.message || 'Unknown error';
        cause = responseObj.cause || null;
      } else {
        message = exceptionResponse as string;
      }
    } else if (exception instanceof Error) {
      // Unhandled application errors
      message = exception.message;
      cause = {
        name: exception.name,
        message: exception.message,
        // Include stack trace in development only
        stack: process.env.NODE_ENV === 'development'
          ? exception.stack
          : undefined,
      };
    } else {
      // Unknown error type
      message = String(exception);
    }

    // Build structured error log
    const errorLog: ErrorContext = {
      timestamp: new Date().toISOString(),
      method: request.method,
      url: request.url,
      statusCode: status,
      message,
      error: cause,
      userId: request.user?.idUsuario,
      requestId: request.id || request.headers['x-request-id'],
    };

    // Log at appropriate level
    if (status >= 500) {
      this.logger.error(JSON.stringify(errorLog));
    } else if (status >= 400) {
      this.logger.warn(JSON.stringify(errorLog));
    } else {
      this.logger.log(JSON.stringify(errorLog));
    }

    // Send response to client
    response.status(status).json({
      statusCode: status,
      message,
      timestamp: new Date().toISOString(),
      path: request.url,
      // Only expose error details in development
      ...(process.env.NODE_ENV === 'development' && { error: cause }),
    });
  }
}
```

Register in `src/main.ts`:

```typescript
import { GlobalExceptionFilter } from './common/filters/global-exception.filter';

async function bootstrap() {
  const app = await NestFactory.create<NestFastifyApplication>(
    AppModule,
    new FastifyAdapter(),
  );

  // Register global exception filter
  app.useGlobalFilters(new GlobalExceptionFilter());

  // ... rest of setup
  await app.listen(port, '0.0.0.0');
}
```

### 1.3 Solution: Structured Logging with Winston

While NestJS has a built-in logger, production applications benefit from structured logging libraries like **Winston** or **Pino**.

#### Why Structured Logging?
- **JSON formatting**: Logs are machine-parseable, not just human-readable
- **Log levels**: ERROR, WARN, INFO, DEBUG, VERBOSE
- **File rotation**: Automatically manages log files
- **Performance**: Non-blocking, async logging
- **Aggregation**: Easily send logs to external services (ELK, Datadog, etc.)

#### Installation:

```bash
npm install winston @nestjs/winston
```

#### Implementation:

Create `src/common/logger/logger.module.ts`:

```typescript
import { Module } from '@nestjs/common';
import { WinstonModule } from '@nestjs/winston';
import * as winston from 'winston';

const logger = WinstonModule.createLogger({
  transports: [
    // Console output for development/debugging
    new winston.transports.Console({
      format: winston.format.combine(
        winston.format.timestamp(),
        winston.format.colorize(),
        winston.format.simple(),
      ),
    }),

    // File output for all logs
    new winston.transports.File({
      filename: 'logs/error.log',
      level: 'error',
      format: winston.format.combine(
        winston.format.timestamp(),
        winston.format.json(),
      ),
    }),

    new winston.transports.File({
      filename: 'logs/combined.log',
      format: winston.format.combine(
        winston.format.timestamp(),
        winston.format.json(),
      ),
    }),
  ],
});

@Module({
  providers: [
    {
      provide: 'WINSTON_MODULE_NEST_PROVIDER',
      useValue: logger,
    },
  ],
  exports: ['WINSTON_MODULE_NEST_PROVIDER'],
})
export class LoggerModule {}
```

Usage in services:

```typescript
import { Inject, Injectable } from '@nestjs/common';
import { Logger } from 'winston';

@Injectable()
export class OrcamentoService {
  constructor(
    @Inject('WINSTON_MODULE_NEST_PROVIDER') private logger: Logger,
    private prisma: PrismaService,
  ) {}

  async list(filters: OrcamentoFilters, userId: number) {
    this.logger.info('Listing orcamentos', {
      userId,
      filters: JSON.stringify(filters),
    });

    try {
      const result = await this.prisma.orcamento.findMany(/* ... */);

      this.logger.info('Successfully listed orcamentos', {
        userId,
        count: result.length,
      });

      return result;
    } catch (error) {
      this.logger.error('Error listing orcamentos', {
        userId,
        error: error.message,
        stack: error.stack,
      });
      throw error;
    }
  }
}
```

### 1.4 Solution: Logging Interceptor for Request Lifecycle

A logging interceptor tracks the full lifecycle of each HTTP request, showing:
- When request started
- How long it took
- Response status
- Any errors that occurred

#### Implementation:

Create `src/common/interceptors/logging.interceptor.ts`:

```typescript
import {
  Injectable,
  NestInterceptor,
  ExecutionContext,
  Logger,
  Inject,
} from '@nestjs/common';
import { Observable } from 'rxjs';
import { tap, catchError } from 'rxjs/operators';
import { Logger as WinstonLogger } from 'winston';

@Injectable()
export class LoggingInterceptor implements NestInterceptor {
  private readonly defaultLogger = new Logger('HTTP');

  constructor(
    @Inject('WINSTON_MODULE_NEST_PROVIDER')
    private winstonLogger: WinstonLogger,
  ) {}

  intercept(context: ExecutionContext, next): Observable<any> {
    const request = context.switchToHttp().getRequest();
    const startTime = Date.now();
    const requestId = request.id || `${Date.now()}-${Math.random()}`;

    const logData = {
      requestId,
      method: request.method,
      url: request.url,
      userId: request.user?.idUsuario,
      userType: request.user?.tipo,
    };

    this.winstonLogger.info('Request started', logData);

    return next.handle().pipe(
      tap(() => {
        const duration = Date.now() - startTime;
        this.winstonLogger.info('Request completed', {
          ...logData,
          statusCode: context.switchToHttp().getResponse().statusCode,
          duration,
        });
      }),
      catchError((error) => {
        const duration = Date.now() - startTime;
        this.winstonLogger.error('Request failed', {
          ...logData,
          duration,
          error: error.message,
          stack: error.stack,
        });
        throw error;
      }),
    );
  }
}
```

Register in `src/main.ts`:

```typescript
import { LoggingInterceptor } from './common/interceptors/logging.interceptor';

async function bootstrap() {
  const app = await NestFactory.create<NestFastifyApplication>(
    AppModule,
    new FastifyAdapter(),
  );

  app.useGlobalInterceptors(new LoggingInterceptor());

  // ... rest of setup
}
```

### 1.5 Environment-Specific Logging

Adjust logging based on environment:

```typescript
// src/common/logger/logger.factory.ts
import * as winston from 'winston';

export function createLogger() {
  const transports = [
    new winston.transports.Console({
      format:
        process.env.NODE_ENV === 'production'
          ? winston.format.json()
          : winston.format.combine(
              winston.format.colorize(),
              winston.format.simple(),
            ),
    }),
  ];

  // Only log to files in production
  if (process.env.NODE_ENV === 'production') {
    transports.push(
      new winston.transports.File({
        filename: 'logs/error.log',
        level: 'error',
        maxsize: 5242880, // 5MB
        maxFiles: 5,
        format: winston.format.json(),
      }),
      new winston.transports.File({
        filename: 'logs/combined.log',
        maxsize: 5242880,
        maxFiles: 5,
        format: winston.format.json(),
      }),
    );
  }

  return winston.createLogger({
    level: process.env.LOG_LEVEL || 'info',
    transports,
  });
}
```

---

## Part 2: TypeScript Type Safety & Validation

### 2.1 Current Problems

The current approach has type safety issues:
- **`any` types**: Used to bypass TypeScript checks
- **No compile-time validation**: TypeScript strict mode not enabled
- **Runtime errors**: Data validation happens too late (after it causes problems)
- **BigInt serialization**: Issues that could be caught by better typing
- **No input validation**: Trust user input without checking

### 2.2 Solution: Enable Strict TypeScript Mode

Strict mode enables multiple TypeScript compiler checks that catch bugs early.

#### Update `tsconfig.json`:

```json
{
  "compilerOptions": {
    // Strict mode enables all strict type checking options
    "strict": true,

    // Additional strict options (included in "strict": true)
    "strictNullChecks": true,        // null/undefined must be explicitly typed
    "strictFunctionTypes": true,     // Stricter function type checking
    "strictBindCallApply": true,     // Stricter bind/call/apply checks
    "strictPropertyInitialization": true, // Class properties must be initialized

    // Implicit any checking
    "noImplicitAny": true,           // Error on variables with implicit any type
    "noImplicitThis": true,          // Error on this with implicit any type

    // Other helpful options
    "noUnusedLocals": true,          // Error on unused local variables
    "noUnusedParameters": true,      // Error on unused parameters
    "noImplicitReturns": true,       // Error when not all code paths return
    "noFallthroughCasesInSwitch": true,

    "skipLibCheck": true,
    "forceConsistentCasingInFileNames": true,
  }
}
```

#### Before (with `any` types):

```typescript
async list(filters: any, userId: number): Promise<any> {
  const result = await this.prisma.$queryRawUnsafe(
    `SELECT * FROM orcamento WHERE ...`,
    ...params
  );

  // TypeScript doesn't know what fields exist
  return result;
}
```

#### After (with strict typing):

```typescript
async list(
  filters: OrcamentoFilters,
  userId: number,
  userType: UserType,
): Promise<OrcamentoListItem[]> {
  // TypeScript knows exactly what type is returned
  const result = await this.prisma.$queryRaw<OrcamentoListItem[]>`
    SELECT ... FROM orcamento ...
  `;

  return result;
}
```

### 2.3 Solution: Use Prisma's Type Utilities

Prisma provides powerful type utilities for type-safe queries.

#### Type Safety Features:

```typescript
import { Prisma } from '@prisma/client';

// 1. Use $queryRaw with generic types (eliminates `any`)
async function getOrcamentos() {
  const result = await prisma.$queryRaw<OrcamentoListItem[]>`
    SELECT * FROM orcamento
  `;
  // result is OrcamentoListItem[], not any
}

// 2. Use Args<> to get input types for a model operation
type FindManyArgs = Prisma.OrcamentoFindManyArgs;
// Gives you the exact type for findMany inputs

// 3. Use Result<> to get output types
type FindManyResult = Prisma.OrcamentoFindManyArgs;
// Gives you the exact return type

// 4. Use Exact<> to enforce strict type compliance
type StrictInput = Prisma.Exact<Input, Shape>;
// Ensures Input strictly matches Shape (no extra properties)
```

#### Real-world Example:

```typescript
import { Prisma } from '@prisma/client';

interface OrcamentoListInput {
  filters: OrcamentoFilters;
  userId: number;
  userType: UserType;
}

async list(input: Prisma.Exact<OrcamentoListInput, OrcamentoListInput>) {
  // Now TypeScript ensures:
  // - filters has the right shape
  // - userId is a number
  // - userType is a valid UserType enum

  const result = await this.prisma.$queryRaw<OrcamentoListItem[]>`
    SELECT ... FROM orcamento WHERE ...
  `;

  return result;
}
```

### 2.4 Solution: Combine Zod + Prisma with zod-prisma-types

zod-prisma-types automatically generates Zod schemas from your Prisma models, providing both compile-time AND runtime validation.

#### Installation:

```bash
npm install zod-prisma-types
```

#### Add to Prisma schema:

In `prisma/schema.prisma`, add the generator:

```prisma
generator zod {
  provider = "zod-prisma-types"
  output = "./zod"
}

model Orcamento {
  /// @zod.string.min(1).describe("Código do orçamento")
  idOrcamento String @id @db.VarChar(20)

  /// @zod.number.int().positive().describe("ID da loja")
  idLoja Int

  /// @zod.number.int().positive().describe("ID do usuário vendedor")
  idUsuario Int

  /// @zod.string.describe("Status do orçamento")
  status String @db.VarChar(20)

  /// @zod.number.multipleOf(0.01).describe("Valor total")
  total Decimal @db.Decimal(10, 2)

  // ... other fields
}
```

#### Generate schemas:

```bash
npx prisma generate
```

This creates `prisma/zod/index.ts` with auto-generated schemas:

```typescript
// Auto-generated by zod-prisma-types
export const OrcamentoSchema = z.object({
  idOrcamento: z.string().min(1),
  idLoja: z.number().int().positive(),
  idUsuario: z.number().int().positive(),
  status: z.string(),
  total: z.number().multipleOf(0.01),
});

export type Orcamento = z.infer<typeof OrcamentoSchema>;
```

#### Use in services:

```typescript
import { OrcamentoSchema } from '@prisma/zod';

async function validateAndProcess(data: unknown) {
  // Validates at runtime
  const validated = OrcamentoSchema.parse(data);

  // TypeScript now knows validated has the right type
  console.log(validated.idOrcamento); // string
  console.log(validated.total); // number
}
```

### 2.5 Solution: Create Proper DTOs with Runtime Validation

DTOs (Data Transfer Objects) define the shape of data coming into your application, with validation at the boundary.

#### Implementation:

Create `src/modules/orcamento/dto/orcamento-filters.dto.ts`:

```typescript
import { z } from 'zod';

/**
 * Schema for budget list filters.
 * This ensures:
 * - Compile-time type safety (TypeScript)
 * - Runtime validation (Zod)
 * - Clear documentation of what inputs are allowed
 */
export const OrcamentoFiltersSchema = z.object({
  // Optional filters - each has validation rules
  idLoja: z
    .number()
    .int()
    .positive()
    .optional()
    .describe('Filter by store ID'),

  idVendedor: z
    .number()
    .int()
    .positive()
    .optional()
    .describe('Filter by vendor/consultant ID'),

  statuses: z
    .array(
      z.enum([
        'ATIVO',
        'EXPIRADO',
        'FECHADO',
        'PERDIDO',
        'CANCELADO',
        'REPLICADO',
      ]),
    )
    .optional()
    .describe('Filter by budget status'),

  mesAno: z
    .string()
    .regex(/^\d{4}-\d{2}$/, 'Format must be YYYY-MM')
    .optional()
    .describe('Filter by month and year'),

  semaforo: z
    .number()
    .min(1)
    .max(3)
    .optional()
    .describe('Filter by follow-up temperature: 1=Hot, 2=Warm, 3=Cold'),

  fornecedor: z
    .string()
    .optional()
    .describe('Filter by supplier name'),

  search: z
    .string()
    .min(1)
    .optional()
    .describe('Search by budget code, vendor, client, or professional'),

  apenasPropriosOrcamentos: z
    .boolean()
    .optional()
    .describe('Show only current user\'s budgets'),
});

// Extract TypeScript type from Zod schema
export type OrcamentoFilters = z.infer<typeof OrcamentoFiltersSchema>;
```

Create `src/modules/orcamento/pipes/orcamento-filters.pipe.ts`:

```typescript
import { PipeTransform, Injectable, BadRequestException } from '@nestjs/common';
import { OrcamentoFiltersSchema } from '../dto/orcamento-filters.dto';
import { ZodError } from 'zod';

/**
 * Pipe to validate and transform OrcamentoFilters.
 * Automatically validates input against schema and provides
 * clear error messages if validation fails.
 */
@Injectable()
export class OrcamentoFiltersPipe implements PipeTransform {
  transform(value: unknown) {
    try {
      return OrcamentoFiltersSchema.parse(value);
    } catch (error) {
      if (error instanceof ZodError) {
        const messages = error.errors.map(err => ({
          field: err.path.join('.'),
          message: err.message,
        }));
        throw new BadRequestException({
          statusCode: 400,
          message: 'Invalid filter parameters',
          errors: messages,
        });
      }
      throw error;
    }
  }
}
```

Use in controller:

```typescript
import { OrcamentoFiltersPipe } from './pipes/orcamento-filters.pipe';

@Controller('orcamento')
export class OrcamentoController {
  constructor(private orcamentoService: OrcamentoService) {}

  @Post('list')
  async list(
    @Body(OrcamentoFiltersPipe) filters: OrcamentoFilters,
    @User() user: AuthUser,
  ) {
    // filters is guaranteed to be valid at this point
    return this.orcamentoService.list(
      filters,
      user.idUsuario,
      user.tipo,
      user.idLoja,
    );
  }
}
```

### 2.6 Solution: Type-Safe Database Queries

Handle Prisma's type safety features properly.

#### Proper BigInt Handling:

Instead of manual conversion in service, use a utility:

```typescript
// src/common/utils/prisma.util.ts
/**
 * Normalize Prisma query results by converting BigInt to number.
 * This is needed because Prisma returns BigInt for large integers,
 * but JSON.stringify cannot serialize BigInt.
 */
export function normalizePrismaResult<T>(data: unknown): T {
  return JSON.parse(
    JSON.stringify(data, (_, value) =>
      typeof value === 'bigint' ? Number(value) : value,
    ),
  ) as T;
}

// Usage in service
async list(...): Promise<OrcamentoListItem[]> {
  const result = await this.prisma.$queryRaw<OrcamentoListItem[]>`
    SELECT ... FROM orcamento ...
  `;

  // Automatically convert BigInt values to numbers
  return normalizePrismaResult<OrcamentoListItem[]>(result);
}
```

#### Type-Safe Select/Include:

```typescript
// Always specify which fields you need
async getOrcamento(id: string) {
  const orcamento = await this.prisma.orcamento.findUnique({
    where: { idOrcamento: id },
    select: {
      idOrcamento: true,
      idLoja: true,
      status: true,
      total: true,
      // NOT including large text fields that aren't needed
      observacao: false,
    },
  });

  return orcamento; // Type is { idOrcamento: string; ... }
}
```

---

## Part 3: Implementation Roadmap

### Phase 1: Enable Strict TypeScript (Priority: HIGH)
1. Update `tsconfig.json` with strict mode
2. Fix compilation errors (may be many initially)
3. Replace `any` types with proper types
4. Update existing services/controllers

**Effort**: 2-4 hours
**Impact**: Catch bugs at compile time, prevent future type errors

### Phase 2: Implement Global Error Handling (Priority: HIGH)
1. Create `GlobalExceptionFilter`
2. Create `LoggingInterceptor`
3. Register both in `main.ts`
4. Test error scenarios

**Effort**: 1-2 hours
**Impact**: Centralized error handling, much easier to debug

### Phase 3: Add Structured Logging (Priority: MEDIUM)
1. Install Winston
2. Create logger module
3. Update services to use Winston logger
4. Remove manual console.log statements

**Effort**: 2-3 hours
**Impact**: Better production observability, easier to trace requests

### Phase 4: Add Input Validation (Priority: MEDIUM)
1. Create DTO files with Zod schemas
2. Create validation pipes
3. Update controllers to use pipes
4. Add zod-prisma-types generator

**Effort**: 3-4 hours
**Impact**: Prevent invalid data from entering system, clearer error messages

### Phase 5: Fix Type Safety (Priority: LOW)
1. Update OrcamentoService with proper types
2. Remove manual BigInt conversion
3. Update all raw queries to use generic types
4. Add type utilities where needed

**Effort**: 1-2 hours
**Impact**: Type-safe database queries, easier to maintain

---

## Part 4: Expected Improvements

### Before (Current State)
```
Error: 500 Internal Server Error
"Do not know how to serialize a BigInt"

Debugging: Check console logs, search through unstructured logs
Time to fix: 30+ minutes
Confidence: Low
```

### After (With Improvements)
```
2025-12-11T14:30:45.123Z [ERROR] Request failed
{
  "requestId": "abc123",
  "method": "POST",
  "url": "/trpc/orcamento.list",
  "statusCode": 500,
  "message": "BigInt serialization error",
  "error": {
    "name": "TypeError",
    "stack": "..."
  },
  "userId": 181,
  "duration": 45
}

Debugging: Parse JSON logs, filter by requestId, see full context
Time to fix: 5 minutes
Confidence: High
```

### Metrics
- **Debugging time**: 30min → 5min (83% reduction)
- **Error detection**: Runtime → Compile time (prevents errors entirely)
- **Data integrity**: No validation → Zod validation (prevents invalid data)
- **Maintainability**: `any` types → Strong typing (easier to refactor)

---

## References

- [NestJS Logger Documentation](https://docs.nestjs.io/techniques/logger)
- [NestJS Exception Filters](https://docs.nestjs.io/exception-filters)
- [NestJS Interceptors](https://docs.nestjs.io/interceptors)
- [Prisma Type Safety](https://www.prisma.io/docs/orm/prisma-client/type-safety)
- [zod-prisma-types](https://www.npmjs.com/package/zod-prisma-types)
- [Winston Logger](https://github.com/winstonjs/winston)
- [Zod Validation](https://zod.dev)
- [Error Handling Best Practices](https://betterstack.com/community/guides/scaling-nodejs/error-handling-nestjs/)
