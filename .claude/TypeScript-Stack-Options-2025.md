# TypeScript Stack Options for ERP Migration

## Overview

This document explores TypeScript stack options for migrating the ERP Staccato from C++/Qt to web. The goal is to select a stack that provides:

- **Type Safety**: Catch errors at compile time
- **Developer Experience**: Fast iteration, good tooling
- **Scalability**: Handle ERP complexity and growth
- **Maintainability**: Clear patterns for large codebase

**Database Note**: We will keep MySQL for the initial migration, with a future migration to PostgreSQL planned.

---

## Backend Framework Comparison

### Express.js

The original Node.js framework. Minimal, flexible, unopinionated.

```typescript
// Express - minimal structure, you build everything
import express from 'express';

const app = express();

app.get('/api/clientes/:id', async (req, res) => {
  try {
    const cliente = await db.query('SELECT * FROM clientes WHERE id = ?', [req.params.id]);
    res.json(cliente);
  } catch (error) {
    res.status(500).json({ error: 'Internal server error' });
  }
});

app.listen(3000);
```

| Aspect | Rating | Notes |
|--------|--------|-------|
| Performance | ⭐⭐⭐ | Moderate, outpaced by Fastify |
| Structure | ⭐⭐ | None built-in, DIY |
| TypeScript | ⭐⭐⭐ | Supported, but not native |
| Learning Curve | ⭐⭐⭐⭐⭐ | Very easy |
| Enterprise Ready | ⭐⭐ | Requires many additions |

**Best For**: Quick prototypes, small APIs, learning Node.js

**Not Ideal For**: Large ERP applications (lacks structure)

---

### Fastify

Performance-focused framework. 2x faster than Express.

```typescript
// Fastify - fast with built-in validation
import Fastify from 'fastify';

const fastify = Fastify({ logger: true });

// Built-in schema validation
const getClienteSchema = {
  params: {
    type: 'object',
    properties: {
      id: { type: 'integer' }
    },
    required: ['id']
  },
  response: {
    200: {
      type: 'object',
      properties: {
        id: { type: 'integer' },
        nome: { type: 'string' },
        cpf_cnpj: { type: 'string' }
      }
    }
  }
};

fastify.get('/api/clientes/:id', { schema: getClienteSchema }, async (request, reply) => {
  const { id } = request.params as { id: number };
  const cliente = await db.query('SELECT * FROM clientes WHERE id = ?', [id]);
  return cliente;
});

fastify.listen({ port: 3000 });
```

| Aspect | Rating | Notes |
|--------|--------|-------|
| Performance | ⭐⭐⭐⭐⭐ | Fastest Node.js framework |
| Structure | ⭐⭐⭐ | Plugin-based, flexible |
| TypeScript | ⭐⭐⭐⭐ | Good support |
| Learning Curve | ⭐⭐⭐⭐ | Moderate |
| Enterprise Ready | ⭐⭐⭐ | Good, but less opinionated |

**Best For**: High-performance APIs, microservices, serverless

**Consideration**: Can be used as NestJS's HTTP adapter (best of both worlds)

---

### NestJS

Enterprise-grade framework with Angular-inspired architecture.

```typescript
// NestJS - structured, enterprise patterns

// cliente.entity.ts - Domain model
@Entity()
export class Cliente {
  @PrimaryGeneratedColumn()
  id: number;

  @Column()
  nome: string;

  @Column({ unique: true })
  cpf_cnpj: string;

  @Column({ type: 'enum', enum: TipoPessoa })
  tipo_pessoa: TipoPessoa;
}

// cliente.dto.ts - Data Transfer Objects with validation
export class CreateClienteDto {
  @IsString()
  @MinLength(3)
  nome: string;

  @IsString()
  @IsCpfCnpj() // Custom validator
  cpf_cnpj: string;

  @IsEnum(TipoPessoa)
  tipo_pessoa: TipoPessoa;
}

// cliente.service.ts - Business logic
@Injectable()
export class ClienteService {
  constructor(
    @InjectRepository(Cliente)
    private clienteRepo: Repository<Cliente>,
  ) {}

  async findOne(id: number): Promise<Cliente> {
    const cliente = await this.clienteRepo.findOne({ where: { id } });
    if (!cliente) {
      throw new NotFoundException(`Cliente ${id} não encontrado`);
    }
    return cliente;
  }

  async create(dto: CreateClienteDto): Promise<Cliente> {
    // Check for duplicate CPF/CNPJ
    const existing = await this.clienteRepo.findOne({
      where: { cpf_cnpj: dto.cpf_cnpj }
    });
    if (existing) {
      throw new ConflictException('CPF/CNPJ já cadastrado');
    }

    const cliente = this.clienteRepo.create(dto);
    return this.clienteRepo.save(cliente);
  }
}

// cliente.controller.ts - HTTP layer
@Controller('clientes')
export class ClienteController {
  constructor(private readonly clienteService: ClienteService) {}

  @Get(':id')
  findOne(@Param('id', ParseIntPipe) id: number) {
    return this.clienteService.findOne(id);
  }

  @Post()
  @UsePipes(new ValidationPipe({ whitelist: true }))
  create(@Body() dto: CreateClienteDto) {
    return this.clienteService.create(dto);
  }
}

// cliente.module.ts - Modular organization
@Module({
  imports: [TypeOrmModule.forFeature([Cliente])],
  controllers: [ClienteController],
  providers: [ClienteService],
  exports: [ClienteService], // Available to other modules
})
export class ClienteModule {}
```

| Aspect | Rating | Notes |
|--------|--------|-------|
| Performance | ⭐⭐⭐⭐ | Good (excellent with Fastify adapter) |
| Structure | ⭐⭐⭐⭐⭐ | Modules, DI, Guards, Pipes, Interceptors |
| TypeScript | ⭐⭐⭐⭐⭐ | Native, first-class support |
| Learning Curve | ⭐⭐⭐ | Steep (similar to Angular) |
| Enterprise Ready | ⭐⭐⭐⭐⭐ | Built for enterprise |

**Best For**: Large applications, teams, complex business logic

**Key Features**:
- Dependency Injection (like Qt's parent-child, but better)
- Modular architecture (organize by feature)
- Built-in validation, guards, interceptors
- Can swap Express for Fastify with one line

```typescript
// main.ts - Switch to Fastify for performance
import { NestFactory } from '@nestjs/core';
import { FastifyAdapter } from '@nestjs/platform-fastify';

const app = await NestFactory.create(AppModule, new FastifyAdapter());
```

---

### Backend Framework Recommendation

**Winner: NestJS with Fastify adapter**

| Criteria | Why NestJS |
|----------|------------|
| ERP Complexity | Modules organize features (Compras, Estoque, Financeiro) |
| Team Scale | Clear patterns, easy onboarding |
| Type Safety | Native TypeScript, decorators, DTOs |
| Performance | Fastify adapter = best of both worlds |
| Testing | Built-in testing utilities |
| Similarity to Qt | Familiar patterns (services, modules, DI) |

---

## Frontend Framework Comparison

### React

Library for building UIs. Flexible, massive ecosystem.

```tsx
// React - flexible, component-based

// hooks/useCliente.ts - Data fetching hook
function useCliente(id: number) {
  const [cliente, setCliente] = useState<Cliente | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<Error | null>(null);

  useEffect(() => {
    fetch(`/api/clientes/${id}`)
      .then(res => res.json())
      .then(setCliente)
      .catch(setError)
      .finally(() => setLoading(false));
  }, [id]);

  return { cliente, loading, error };
}

// components/ClienteForm.tsx
interface ClienteFormProps {
  onSubmit: (data: CreateClienteDto) => void;
  initialData?: Partial<Cliente>;
}

function ClienteForm({ onSubmit, initialData }: ClienteFormProps) {
  const [formData, setFormData] = useState({
    nome: initialData?.nome ?? '',
    cpf_cnpj: initialData?.cpf_cnpj ?? '',
    tipo_pessoa: initialData?.tipo_pessoa ?? TipoPessoa.FISICA,
  });

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    onSubmit(formData);
  };

  return (
    <form onSubmit={handleSubmit}>
      <input
        value={formData.nome}
        onChange={e => setFormData(prev => ({ ...prev, nome: e.target.value }))}
        placeholder="Nome"
      />
      <InputMask
        mask={formData.tipo_pessoa === TipoPessoa.FISICA ? '999.999.999-99' : '99.999.999/9999-99'}
        value={formData.cpf_cnpj}
        onChange={e => setFormData(prev => ({ ...prev, cpf_cnpj: e.target.value }))}
      />
      <button type="submit">Salvar</button>
    </form>
  );
}

// pages/ClienteList.tsx - With data table
function ClienteList() {
  const { data, isLoading } = useQuery(['clientes'], fetchClientes);

  const columns = [
    { header: 'ID', accessorKey: 'id' },
    { header: 'Nome', accessorKey: 'nome' },
    { header: 'CPF/CNPJ', accessorKey: 'cpf_cnpj' },
    {
      header: 'Ações',
      cell: ({ row }) => (
        <div>
          <Button onClick={() => navigate(`/clientes/${row.original.id}`)}>
            Editar
          </Button>
        </div>
      )
    },
  ];

  return (
    <DataTable columns={columns} data={data ?? []} loading={isLoading} />
  );
}
```

| Aspect | Rating | Notes |
|--------|--------|-------|
| Flexibility | ⭐⭐⭐⭐⭐ | Choose your own libraries |
| TypeScript | ⭐⭐⭐⭐⭐ | Excellent support |
| Ecosystem | ⭐⭐⭐⭐⭐ | Largest (TanStack, Zustand, etc.) |
| Hiring | ⭐⭐⭐⭐⭐ | 60% of frontend market |
| Learning Curve | ⭐⭐⭐⭐ | Moderate |
| Structure | ⭐⭐⭐ | You decide (can be good or bad) |

**Popular React Stack for ERP**:
- **TanStack Query**: Server state management
- **TanStack Table**: Powerful data tables
- **React Hook Form + Zod**: Form validation
- **Zustand/Jotai**: Client state
- **Tailwind CSS**: Styling

---

### Angular

Full framework by Google. Batteries included.

```typescript
// Angular - structured, opinionated

// cliente.model.ts
export interface Cliente {
  id: number;
  nome: string;
  cpf_cnpj: string;
  tipo_pessoa: TipoPessoa;
}

// cliente.service.ts
@Injectable({ providedIn: 'root' })
export class ClienteService {
  private apiUrl = '/api/clientes';

  constructor(private http: HttpClient) {}

  getAll(): Observable<Cliente[]> {
    return this.http.get<Cliente[]>(this.apiUrl);
  }

  getById(id: number): Observable<Cliente> {
    return this.http.get<Cliente>(`${this.apiUrl}/${id}`);
  }

  create(cliente: CreateClienteDto): Observable<Cliente> {
    return this.http.post<Cliente>(this.apiUrl, cliente);
  }

  update(id: number, cliente: Partial<Cliente>): Observable<Cliente> {
    return this.http.patch<Cliente>(`${this.apiUrl}/${id}`, cliente);
  }
}

// cliente-form.component.ts
@Component({
  selector: 'app-cliente-form',
  template: `
    <form [formGroup]="form" (ngSubmit)="onSubmit()">
      <mat-form-field>
        <mat-label>Nome</mat-label>
        <input matInput formControlName="nome">
        <mat-error *ngIf="form.get('nome')?.hasError('required')">
          Nome é obrigatório
        </mat-error>
      </mat-form-field>

      <mat-form-field>
        <mat-label>CPF/CNPJ</mat-label>
        <input matInput formControlName="cpf_cnpj" [mask]="cpfCnpjMask">
        <mat-error *ngIf="form.get('cpf_cnpj')?.hasError('invalidCpfCnpj')">
          CPF/CNPJ inválido
        </mat-error>
      </mat-form-field>

      <mat-radio-group formControlName="tipo_pessoa">
        <mat-radio-button value="FISICA">Pessoa Física</mat-radio-button>
        <mat-radio-button value="JURIDICA">Pessoa Jurídica</mat-radio-button>
      </mat-radio-group>

      <button mat-raised-button color="primary" type="submit" [disabled]="form.invalid">
        Salvar
      </button>
    </form>
  `
})
export class ClienteFormComponent implements OnInit {
  form: FormGroup;

  constructor(
    private fb: FormBuilder,
    private clienteService: ClienteService,
    private router: Router
  ) {}

  ngOnInit() {
    this.form = this.fb.group({
      nome: ['', [Validators.required, Validators.minLength(3)]],
      cpf_cnpj: ['', [Validators.required, cpfCnpjValidator]],
      tipo_pessoa: [TipoPessoa.FISICA, Validators.required],
    });
  }

  get cpfCnpjMask(): string {
    return this.form.get('tipo_pessoa')?.value === TipoPessoa.FISICA
      ? '000.000.000-00'
      : '00.000.000/0000-00';
  }

  onSubmit() {
    if (this.form.valid) {
      this.clienteService.create(this.form.value).subscribe({
        next: (cliente) => this.router.navigate(['/clientes', cliente.id]),
        error: (err) => console.error('Erro ao criar cliente', err),
      });
    }
  }
}

// cliente.module.ts
@NgModule({
  declarations: [
    ClienteListComponent,
    ClienteFormComponent,
    ClienteDetailComponent,
  ],
  imports: [
    CommonModule,
    ReactiveFormsModule,
    MaterialModule,
    ClienteRoutingModule,
  ],
  providers: [ClienteService],
})
export class ClienteModule {}
```

| Aspect | Rating | Notes |
|--------|--------|-------|
| Flexibility | ⭐⭐⭐ | Opinionated, less choice |
| TypeScript | ⭐⭐⭐⭐⭐ | Required, best integration |
| Ecosystem | ⭐⭐⭐⭐ | Angular Material, CDK |
| Hiring | ⭐⭐⭐⭐ | Good, especially enterprise |
| Learning Curve | ⭐⭐ | Steep (RxJS, decorators, modules) |
| Structure | ⭐⭐⭐⭐⭐ | Enforced, consistent |

**Built-in Features**:
- Routing with guards
- Reactive forms with validation
- HTTP client with interceptors
- Dependency injection
- RxJS for reactive programming

---

### Vue

Progressive framework. Balance of structure and flexibility.

```typescript
// Vue 3 + Composition API + TypeScript

// composables/useCliente.ts
export function useCliente(id: Ref<number>) {
  const cliente = ref<Cliente | null>(null);
  const loading = ref(true);
  const error = ref<Error | null>(null);

  watchEffect(async () => {
    loading.value = true;
    try {
      const response = await fetch(`/api/clientes/${id.value}`);
      cliente.value = await response.json();
    } catch (e) {
      error.value = e as Error;
    } finally {
      loading.value = false;
    }
  });

  return { cliente, loading, error };
}

// components/ClienteForm.vue
<script setup lang="ts">
import { ref, computed } from 'vue';
import { useVModel } from '@vueuse/core';

interface Props {
  modelValue: CreateClienteDto;
}

const props = defineProps<Props>();
const emit = defineEmits<{
  'update:modelValue': [value: CreateClienteDto];
  'submit': [value: CreateClienteDto];
}>();

const formData = useVModel(props, 'modelValue', emit);

const cpfCnpjMask = computed(() =>
  formData.value.tipo_pessoa === TipoPessoa.FISICA
    ? '###.###.###-##'
    : '##.###.###/####-##'
);

const handleSubmit = () => {
  emit('submit', formData.value);
};
</script>

<template>
  <form @submit.prevent="handleSubmit">
    <div class="form-group">
      <label>Nome</label>
      <input v-model="formData.nome" required minlength="3" />
    </div>

    <div class="form-group">
      <label>CPF/CNPJ</label>
      <input v-model="formData.cpf_cnpj" v-mask="cpfCnpjMask" />
    </div>

    <div class="form-group">
      <label>Tipo</label>
      <select v-model="formData.tipo_pessoa">
        <option :value="TipoPessoa.FISICA">Pessoa Física</option>
        <option :value="TipoPessoa.JURIDICA">Pessoa Jurídica</option>
      </select>
    </div>

    <button type="submit">Salvar</button>
  </form>
</template>
```

| Aspect | Rating | Notes |
|--------|--------|-------|
| Flexibility | ⭐⭐⭐⭐ | Good balance |
| TypeScript | ⭐⭐⭐⭐ | Good with Composition API |
| Ecosystem | ⭐⭐⭐⭐ | Pinia, VueUse, Nuxt |
| Hiring | ⭐⭐⭐ | Smaller pool in Brazil |
| Learning Curve | ⭐⭐⭐⭐⭐ | Easiest to learn |
| Structure | ⭐⭐⭐⭐ | SFC pattern, Composition API |

---

### Frontend Framework Recommendation

**For ERP Staccato: React or Angular**

| Criteria | React | Angular |
|----------|-------|---------|
| Hiring in Brazil | ⭐⭐⭐⭐⭐ Larger pool | ⭐⭐⭐⭐ Good for enterprise |
| TypeScript | Excellent | Required (best) |
| Data Tables | TanStack Table | Material Table |
| Forms | React Hook Form | Reactive Forms (built-in) |
| Structure | You define | Enforced |
| NestJS Similarity | Different patterns | Very similar patterns |

**Recommendation**:
- **React** if you want flexibility and maximum hiring pool
- **Angular** if you want enforced structure matching NestJS

---

## ORM Comparison

### Prisma

Schema-first ORM with best developer experience.

```typescript
// prisma/schema.prisma
datasource db {
  provider = "mysql"  // Will change to "postgresql" in future
  url      = env("DATABASE_URL")
}

generator client {
  provider = "prisma-client-js"
}

model Cliente {
  id          Int         @id @default(autoincrement())
  nome        String      @db.VarChar(255)
  cpf_cnpj    String      @unique @db.VarChar(18)
  tipo_pessoa TipoPessoa
  ativo       Boolean     @default(true)
  created_at  DateTime    @default(now())
  updated_at  DateTime    @updatedAt

  // Relations
  enderecos   Endereco[]
  pedidos     Pedido[]
  orcamentos  Orcamento[]

  @@map("clientes")
}

model Orcamento {
  id            Int             @id @default(autoincrement())
  numero        String          @unique
  cliente_id    Int
  status        StatusOrcamento @default(ABERTO)
  valor_total   Decimal         @db.Decimal(15, 2)
  created_at    DateTime        @default(now())

  cliente       Cliente         @relation(fields: [cliente_id], references: [id])
  itens         OrcamentoItem[]

  @@map("orcamentos")
}

enum TipoPessoa {
  FISICA
  JURIDICA
}

enum StatusOrcamento {
  ABERTO
  APROVADO
  CANCELADO
  CONVERTIDO
}
```

```typescript
// Usage in NestJS service
@Injectable()
export class OrcamentoService {
  constructor(private prisma: PrismaService) {}

  // Prisma provides full type safety
  async findAll(filters: OrcamentoFilters) {
    return this.prisma.orcamento.findMany({
      where: {
        status: filters.status,
        cliente: {
          nome: { contains: filters.clienteNome },
        },
        created_at: {
          gte: filters.dataInicio,
          lte: filters.dataFim,
        },
      },
      include: {
        cliente: true,
        itens: {
          include: {
            produto: true,
          },
        },
      },
      orderBy: { created_at: 'desc' },
      skip: filters.page * filters.pageSize,
      take: filters.pageSize,
    });
  }

  // Type-safe transactions
  async aprovarOrcamento(id: number) {
    return this.prisma.$transaction(async (tx) => {
      const orcamento = await tx.orcamento.findUnique({
        where: { id },
        include: { itens: true },
      });

      if (!orcamento) {
        throw new NotFoundException('Orçamento não encontrado');
      }

      if (orcamento.status !== 'ABERTO') {
        throw new BadRequestException('Orçamento não está aberto');
      }

      // Update status
      return tx.orcamento.update({
        where: { id },
        data: { status: 'APROVADO' },
      });
    });
  }
}
```

| Aspect | Rating | Notes |
|--------|--------|-------|
| Type Safety | ⭐⭐⭐⭐⭐ | Generated types from schema |
| DX | ⭐⭐⭐⭐⭐ | Best autocomplete, Prisma Studio |
| Performance | ⭐⭐⭐⭐ | Good, Rust query engine |
| Raw SQL | ⭐⭐⭐ | Supported but less elegant |
| Migrations | ⭐⭐⭐⭐⭐ | Excellent tooling |
| MySQL Support | ⭐⭐⭐⭐⭐ | Full support |

---

### Drizzle

SQL-first ORM with TypeScript. Zero dependencies.

```typescript
// db/schema.ts
import { mysqlTable, int, varchar, decimal, datetime, mysqlEnum, boolean } from 'drizzle-orm/mysql-core';

export const clientes = mysqlTable('clientes', {
  id: int('id').primaryKey().autoincrement(),
  nome: varchar('nome', { length: 255 }).notNull(),
  cpf_cnpj: varchar('cpf_cnpj', { length: 18 }).unique().notNull(),
  tipo_pessoa: mysqlEnum('tipo_pessoa', ['FISICA', 'JURIDICA']).notNull(),
  ativo: boolean('ativo').default(true),
  created_at: datetime('created_at').defaultNow(),
});

export const orcamentos = mysqlTable('orcamentos', {
  id: int('id').primaryKey().autoincrement(),
  numero: varchar('numero', { length: 20 }).unique().notNull(),
  cliente_id: int('cliente_id').references(() => clientes.id),
  status: mysqlEnum('status', ['ABERTO', 'APROVADO', 'CANCELADO', 'CONVERTIDO']).default('ABERTO'),
  valor_total: decimal('valor_total', { precision: 15, scale: 2 }),
  created_at: datetime('created_at').defaultNow(),
});

export const orcamentoItens = mysqlTable('orcamento_itens', {
  id: int('id').primaryKey().autoincrement(),
  orcamento_id: int('orcamento_id').references(() => orcamentos.id),
  produto_id: int('produto_id').references(() => produtos.id),
  quantidade: decimal('quantidade', { precision: 15, scale: 4 }),
  valor_unitario: decimal('valor_unitario', { precision: 15, scale: 2 }),
});
```

```typescript
// Usage - SQL-like syntax
@Injectable()
export class OrcamentoService {
  constructor(@Inject('DB') private db: MySql2Database) {}

  async findAll(filters: OrcamentoFilters) {
    // SQL-like query builder
    return this.db
      .select({
        id: orcamentos.id,
        numero: orcamentos.numero,
        status: orcamentos.status,
        valor_total: orcamentos.valor_total,
        cliente_nome: clientes.nome,
      })
      .from(orcamentos)
      .leftJoin(clientes, eq(orcamentos.cliente_id, clientes.id))
      .where(
        and(
          filters.status ? eq(orcamentos.status, filters.status) : undefined,
          filters.clienteNome ? like(clientes.nome, `%${filters.clienteNome}%`) : undefined,
        )
      )
      .orderBy(desc(orcamentos.created_at))
      .limit(filters.pageSize)
      .offset(filters.page * filters.pageSize);
  }

  // Raw SQL when needed - fully typed
  async getRelatorioVendas(dataInicio: Date, dataFim: Date) {
    return this.db.execute(sql`
      SELECT
        DATE(created_at) as data,
        COUNT(*) as total_orcamentos,
        SUM(valor_total) as valor_total
      FROM orcamentos
      WHERE created_at BETWEEN ${dataInicio} AND ${dataFim}
        AND status = 'APROVADO'
      GROUP BY DATE(created_at)
      ORDER BY data
    `);
  }
}
```

| Aspect | Rating | Notes |
|--------|--------|-------|
| Type Safety | ⭐⭐⭐⭐⭐ | Inferred from schema |
| DX | ⭐⭐⭐⭐ | Good, SQL-like |
| Performance | ⭐⭐⭐⭐⭐ | Fastest, zero overhead |
| Raw SQL | ⭐⭐⭐⭐⭐ | Native, typed |
| Migrations | ⭐⭐⭐⭐ | Drizzle Kit |
| MySQL Support | ⭐⭐⭐⭐⭐ | Full support |

---

### TypeORM

Traditional ORM with decorators. Active Record or Data Mapper patterns.

```typescript
// entities/cliente.entity.ts
@Entity('clientes')
export class Cliente {
  @PrimaryGeneratedColumn()
  id: number;

  @Column({ length: 255 })
  nome: string;

  @Column({ unique: true, length: 18 })
  cpf_cnpj: string;

  @Column({ type: 'enum', enum: TipoPessoa })
  tipo_pessoa: TipoPessoa;

  @Column({ default: true })
  ativo: boolean;

  @CreateDateColumn()
  created_at: Date;

  @UpdateDateColumn()
  updated_at: Date;

  @OneToMany(() => Orcamento, (orcamento) => orcamento.cliente)
  orcamentos: Orcamento[];
}

// Usage
@Injectable()
export class OrcamentoService {
  constructor(
    @InjectRepository(Orcamento)
    private orcamentoRepo: Repository<Orcamento>,
  ) {}

  async findAll(filters: OrcamentoFilters) {
    const qb = this.orcamentoRepo
      .createQueryBuilder('o')
      .leftJoinAndSelect('o.cliente', 'c')
      .leftJoinAndSelect('o.itens', 'i')
      .leftJoinAndSelect('i.produto', 'p');

    if (filters.status) {
      qb.andWhere('o.status = :status', { status: filters.status });
    }

    if (filters.clienteNome) {
      qb.andWhere('c.nome LIKE :nome', { nome: `%${filters.clienteNome}%` });
    }

    return qb
      .orderBy('o.created_at', 'DESC')
      .skip(filters.page * filters.pageSize)
      .take(filters.pageSize)
      .getMany();
  }
}
```

| Aspect | Rating | Notes |
|--------|--------|-------|
| Type Safety | ⭐⭐⭐⭐ | Good, decorator-based |
| DX | ⭐⭐⭐⭐ | Familiar if from Java/C# |
| Performance | ⭐⭐⭐ | Moderate, some overhead |
| Raw SQL | ⭐⭐⭐⭐ | Query builder |
| Migrations | ⭐⭐⭐⭐ | Good, CLI tools |
| MySQL Support | ⭐⭐⭐⭐⭐ | Full support |

---

### ORM Recommendation

**For ERP Staccato: Prisma**

| Criteria | Prisma | Drizzle | TypeORM |
|----------|--------|---------|---------|
| Learning Curve | Easy | Medium | Medium |
| Type Safety | Excellent | Excellent | Good |
| Complex Queries | Good | Excellent | Good |
| Migration Tools | Excellent | Good | Good |
| MySQL → PostgreSQL | Easy | Easy | Easy |
| Team Productivity | Highest | High | Moderate |

**Why Prisma**:
1. Best developer experience for teams
2. Schema is readable documentation
3. Excellent migration tooling
4. Easy database switch (MySQL → PostgreSQL)
5. Prisma Studio for debugging

**Consider Drizzle if**:
- Performance is critical
- Team is comfortable with SQL
- Need complex queries frequently

---

## API Style Comparison

### tRPC

End-to-end type safety without code generation.

```typescript
// server/trpc/routers/cliente.ts
export const clienteRouter = router({
  list: publicProcedure
    .input(z.object({
      page: z.number().default(0),
      pageSize: z.number().default(20),
      search: z.string().optional(),
    }))
    .query(async ({ input, ctx }) => {
      return ctx.prisma.cliente.findMany({
        where: input.search ? {
          OR: [
            { nome: { contains: input.search } },
            { cpf_cnpj: { contains: input.search } },
          ],
        } : undefined,
        skip: input.page * input.pageSize,
        take: input.pageSize,
      });
    }),

  getById: publicProcedure
    .input(z.number())
    .query(async ({ input, ctx }) => {
      const cliente = await ctx.prisma.cliente.findUnique({
        where: { id: input },
      });
      if (!cliente) {
        throw new TRPCError({ code: 'NOT_FOUND' });
      }
      return cliente;
    }),

  create: protectedProcedure
    .input(createClienteSchema)
    .mutation(async ({ input, ctx }) => {
      return ctx.prisma.cliente.create({ data: input });
    }),

  update: protectedProcedure
    .input(z.object({
      id: z.number(),
      data: updateClienteSchema,
    }))
    .mutation(async ({ input, ctx }) => {
      return ctx.prisma.cliente.update({
        where: { id: input.id },
        data: input.data,
      });
    }),
});

// client usage - FULL TYPE SAFETY
// React component
function ClienteList() {
  // Types inferred from server!
  const { data, isLoading } = trpc.cliente.list.useQuery({
    page: 0,
    pageSize: 20,
  });

  const createMutation = trpc.cliente.create.useMutation();

  // TypeScript knows exactly what data.cliente contains
  // No manual type definitions needed!
}
```

| Aspect | Rating | Notes |
|--------|--------|-------|
| Type Safety | ⭐⭐⭐⭐⭐ | Automatic, end-to-end |
| DX | ⭐⭐⭐⭐⭐ | Best for monorepos |
| Performance | ⭐⭐⭐⭐⭐ | Minimal overhead |
| External APIs | ⭐ | Not designed for this |
| Learning Curve | ⭐⭐⭐⭐ | Easy if know TypeScript |

---

### REST

Traditional HTTP APIs with OpenAPI/Swagger.

```typescript
// NestJS REST Controller
@ApiTags('clientes')
@Controller('api/clientes')
export class ClienteController {
  constructor(private clienteService: ClienteService) {}

  @Get()
  @ApiOperation({ summary: 'Listar clientes' })
  @ApiQuery({ name: 'page', required: false })
  @ApiQuery({ name: 'search', required: false })
  @ApiResponse({ status: 200, type: [ClienteDto] })
  async list(
    @Query('page') page = 0,
    @Query('pageSize') pageSize = 20,
    @Query('search') search?: string,
  ) {
    return this.clienteService.list({ page, pageSize, search });
  }

  @Get(':id')
  @ApiOperation({ summary: 'Buscar cliente por ID' })
  @ApiResponse({ status: 200, type: ClienteDto })
  @ApiResponse({ status: 404, description: 'Cliente não encontrado' })
  async getById(@Param('id', ParseIntPipe) id: number) {
    return this.clienteService.getById(id);
  }

  @Post()
  @ApiOperation({ summary: 'Criar cliente' })
  @ApiBody({ type: CreateClienteDto })
  @ApiResponse({ status: 201, type: ClienteDto })
  async create(@Body() dto: CreateClienteDto) {
    return this.clienteService.create(dto);
  }
}

// Client needs generated types or manual definitions
// Using openapi-typescript-codegen or similar
```

| Aspect | Rating | Notes |
|--------|--------|-------|
| Type Safety | ⭐⭐⭐ | Requires codegen |
| DX | ⭐⭐⭐⭐ | Familiar, well understood |
| Performance | ⭐⭐⭐⭐ | Good |
| External APIs | ⭐⭐⭐⭐⭐ | Standard, any client |
| Learning Curve | ⭐⭐⭐⭐⭐ | Everyone knows REST |

---

### GraphQL

Query language for flexible data fetching.

```typescript
// NestJS GraphQL Resolver
@Resolver(() => Cliente)
export class ClienteResolver {
  constructor(private clienteService: ClienteService) {}

  @Query(() => [Cliente])
  async clientes(
    @Args('page', { nullable: true, defaultValue: 0 }) page: number,
    @Args('pageSize', { nullable: true, defaultValue: 20 }) pageSize: number,
    @Args('search', { nullable: true }) search?: string,
  ) {
    return this.clienteService.list({ page, pageSize, search });
  }

  @Query(() => Cliente, { nullable: true })
  async cliente(@Args('id', { type: () => Int }) id: number) {
    return this.clienteService.getById(id);
  }

  @ResolveField(() => [Orcamento])
  async orcamentos(@Parent() cliente: Cliente) {
    return this.orcamentoService.findByCliente(cliente.id);
  }

  @Mutation(() => Cliente)
  async createCliente(@Args('input') input: CreateClienteInput) {
    return this.clienteService.create(input);
  }
}

// Client query
const GET_CLIENTE = gql`
  query GetCliente($id: Int!) {
    cliente(id: $id) {
      id
      nome
      cpf_cnpj
      orcamentos {
        id
        numero
        valor_total
      }
    }
  }
`;
```

| Aspect | Rating | Notes |
|--------|--------|-------|
| Type Safety | ⭐⭐⭐⭐ | With codegen (graphql-codegen) |
| DX | ⭐⭐⭐⭐ | Good with tooling |
| Performance | ⭐⭐⭐ | N+1 problems, caching complex |
| External APIs | ⭐⭐⭐⭐⭐ | Excellent for mobile/multiple clients |
| Learning Curve | ⭐⭐⭐ | Moderate, new concepts |

---

### API Style Recommendation

**For ERP Staccato: tRPC (primary) + REST (where needed)**

| Use Case | Recommendation |
|----------|----------------|
| Internal web app | tRPC |
| Mobile app (future) | REST or GraphQL |
| External integrations | REST |
| ACBr communication | TCP (existing) |

**Why tRPC**:
1. Zero-effort type safety between frontend and backend
2. No code generation step
3. Excellent with React Query
4. Can add REST endpoints later if needed

---

## Recommended Stack Combinations

### Option A: Maximum Type Safety (Recommended)

```
┌─────────────────────────────────────────────────────────────┐
│                        Frontend                             │
│                  React + TypeScript                         │
│          TanStack Query + TanStack Table                    │
│              React Hook Form + Zod                          │
│                    Tailwind CSS                             │
└─────────────────────────┬───────────────────────────────────┘
                          │ tRPC (type-safe)
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                        Backend                              │
│               NestJS + Fastify adapter                      │
│                    tRPC adapter                             │
│                                                             │
│    ┌─────────────┐  ┌─────────────┐  ┌─────────────┐       │
│    │   Prisma    │  │  ACBr TCP   │  │  Services   │       │
│    │    ORM      │  │   Client    │  │  (Business) │       │
│    └──────┬──────┘  └─────────────┘  └─────────────┘       │
│           │                                                 │
└───────────┼─────────────────────────────────────────────────┘
            │
            ▼
     ┌─────────────┐
     │    MySQL    │  → Future: PostgreSQL
     └─────────────┘
```

**Pros**:
- Full end-to-end type safety
- Best developer experience
- Change detection at compile time
- Fastest iteration speed

**Cons**:
- tRPC learning curve
- Less familiar to some developers

**Best For**: Teams committed to TypeScript, rapid development

---

### Option B: Traditional REST (More Familiar)

```
┌─────────────────────────────────────────────────────────────┐
│                        Frontend                             │
│                  React + TypeScript                         │
│          TanStack Query + TanStack Table                    │
│                OpenAPI generated types                      │
│                    Tailwind CSS                             │
└─────────────────────────┬───────────────────────────────────┘
                          │ REST API (OpenAPI/Swagger)
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                        Backend                              │
│               NestJS + Fastify adapter                      │
│                  Swagger/OpenAPI                            │
│                                                             │
│    ┌─────────────┐  ┌─────────────┐  ┌─────────────┐       │
│    │   Prisma    │  │  ACBr TCP   │  │   Guards/   │       │
│    │    ORM      │  │   Client    │  │   Pipes     │       │
│    └──────┬──────┘  └─────────────┘  └─────────────┘       │
│           │                                                 │
└───────────┼─────────────────────────────────────────────────┘
            │
            ▼
     ┌─────────────┐
     │    MySQL    │  → Future: PostgreSQL
     └─────────────┘
```

**Pros**:
- Familiar patterns
- Easy to understand for any developer
- Swagger UI for documentation
- Ready for external integrations

**Cons**:
- Manual type synchronization (or codegen step)
- More boilerplate

**Best For**: Teams with REST experience, need external API

---

### Option C: Angular Full-Stack (Maximum Structure)

```
┌─────────────────────────────────────────────────────────────┐
│                        Frontend                             │
│                       Angular                               │
│               Angular Material + CDK                        │
│                  Reactive Forms                             │
│                      RxJS                                   │
└─────────────────────────┬───────────────────────────────────┘
                          │ REST API (OpenAPI)
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                        Backend                              │
│               NestJS + Fastify adapter                      │
│                  Swagger/OpenAPI                            │
│                                                             │
│    ┌─────────────┐  ┌─────────────┐  ┌─────────────┐       │
│    │   Prisma    │  │  ACBr TCP   │  │   Guards/   │       │
│    │    ORM      │  │   Client    │  │Interceptors │       │
│    └──────┬──────┘  └─────────────┘  └─────────────┘       │
│           │                                                 │
└───────────┼─────────────────────────────────────────────────┘
            │
            ▼
     ┌─────────────┐
     │    MySQL    │  → Future: PostgreSQL
     └─────────────┘
```

**Pros**:
- Both Angular and NestJS use same patterns
- Maximum enforced structure
- Built-in everything
- Excellent for very large teams

**Cons**:
- Steeper learning curve
- More verbose
- Smaller hiring pool than React

**Best For**: Teams wanting strict patterns, enterprise environment

---

## Summary Comparison

| Aspect | Option A (tRPC) | Option B (REST) | Option C (Angular) |
|--------|-----------------|-----------------|-------------------|
| Type Safety | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| Learning Curve | Medium | Easy | Steep |
| Boilerplate | Minimal | Moderate | High |
| Flexibility | High | High | Low |
| Structure | You decide | You decide | Enforced |
| External API Ready | No (add later) | Yes | Yes |
| Hiring Pool | Large | Largest | Medium |

---

## Final Recommendation

**For ERP Staccato: Option A (tRPC + React + NestJS + Prisma)**

Reasons:
1. Maximum type safety catches errors early
2. Fastest development iteration
3. React has largest talent pool
4. Prisma makes database work pleasant
5. Can add REST endpoints later if needed
6. NestJS provides enterprise structure

### Immediate Next Steps

1. Set up monorepo (Turborepo or Nx)
2. Create NestJS backend with Prisma
3. Create React frontend with tRPC client
4. Implement ACBr TCP client service
5. Start migrating module by module

---

## References

### Backend
- [NestJS vs Fastify Comparison](https://betterstack.com/community/guides/scaling-nodejs/nestjs-vs-fastify/)
- [NestJS Documentation](https://docs.nestjs.com/)
- [Fastify Performance](https://www.fastify.io/benchmarks/)

### Frontend
- [React vs Angular vs Vue 2025](https://zerotomastery.io/blog/angular-vs-react-vs-vue/)
- [Enterprise Framework Comparison](https://www.codertrove.com/articles/2025-tech-stack-dilemma-react-vs-vue-vs-angular-for-enterprise-application)

### ORM
- [Prisma vs Drizzle](https://betterstack.com/community/guides/scaling-nodejs/drizzle-vs-prisma/)
- [TypeScript ORM Comparison 2025](https://www.bytebase.com/blog/top-typescript-orm/)

### API
- [tRPC vs GraphQL vs REST](https://wundergraph.com/blog/graphql-vs-federation-vs-trpc-vs-rest-vs-grpc-vs-asyncapi-vs-webhooks)
- [Why tRPC in 2025](https://medium.com/@asierr/why-more-developers-are-choosing-trpc-over-rest-and-graphql-in-2025-8eeb7f9029b5)
