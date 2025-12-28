# Greenfield Business Flow Design

> Status: **Design Proposal**
> Last updated: 2025-12-27
> Philosophy: Simple, Reliable, Testable, Event-Driven

---

## Table of Contents

1. [Design Philosophy](#1-design-philosophy)
2. [Core Concepts](#2-core-concepts)
3. [Bounded Contexts](#3-bounded-contexts)
4. [The Order Lifecycle](#4-the-order-lifecycle)
5. [Inventory Management](#5-inventory-management)
6. [Delivery Management](#6-delivery-management)
7. [Fiscal & Financial](#7-fiscal--financial)
8. [State Machines](#8-state-machines)
9. [Event Architecture](#9-event-architecture)
10. [Database Schema](#10-database-schema)
11. [Testing Strategy](#11-testing-strategy)
12. [Advanced Features](#12-advanced-features)

---

## 1. Design Philosophy

### 1.1 Guiding Principles

| Principle | Meaning |
|-----------|---------|
| **Explicit over implicit** | Every state and transition is named and documented |
| **Events over mutations** | State changes are events, not updates |
| **Derived over duplicated** | Calculate values, don't store copies |
| **Constraints in schema** | Database enforces rules, not just code |
| **Single responsibility** | Each entity does ONE thing well |
| **Testable by design** | Pure functions, no hidden dependencies |

### 1.2 What We're Avoiding

```
❌ Magic strings for status
❌ Mutable state without history
❌ Split tables (L1/L2 pattern)
❌ Denormalized data
❌ Business logic in controllers
❌ Implicit state transitions
❌ Complex conditionals scattered in code
```

### 1.3 What We're Embracing

```
✅ Enums for all statuses
✅ Event sourcing for audit trail
✅ Single table with fulfillment records
✅ Normalized references (FKs everywhere)
✅ Domain services for business logic
✅ State machines with explicit guards
✅ Centralized business rules
```

---

## 2. Core Concepts

### 2.1 The Fulfillment Model

**The Big Insight**: An order item doesn't "split" - it gets **fulfilled** in parts.

```
┌─────────────────────────────────────────────────────────────────┐
│ ORDER ITEM: 10 units of "Mesa de Jantar" @ R$ 500 each          │
│ Status: PARTIALLY_FULFILLED                                      │
├─────────────────────────────────────────────────────────────────┤
│ FULFILLMENTS:                                                    │
│ ┌─────────────────────────────────────────────────────────────┐ │
│ │ #1: 4 units from Stock #123 (Supplier A)                    │ │
│ │     Reserved: 2024-01-05 → Consumed: 2024-01-06             │ │
│ │     Delivered: 2024-01-10 → Invoiced: 2024-01-10            │ │
│ │     Status: INVOICED ✓                                      │ │
│ └─────────────────────────────────────────────────────────────┘ │
│ ┌─────────────────────────────────────────────────────────────┐ │
│ │ #2: 4 units from Stock #456 (Supplier A)                    │ │
│ │     Reserved: 2024-01-08 → Consumed: 2024-01-09             │ │
│ │     Delivery scheduled: 2024-01-15                          │ │
│ │     Status: AWAITING_DELIVERY                               │ │
│ └─────────────────────────────────────────────────────────────┘ │
│ ┌─────────────────────────────────────────────────────────────┐ │
│ │ #3: 2 units - UNFULFILLED                                   │ │
│ │     Waiting for stock (PO #789 expected 2024-01-20)         │ │
│ │     Status: AWAITING_STOCK                                  │ │
│ └─────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

**Why this is better:**

| Current System | Fulfillment Model |
|----------------|-------------------|
| One item becomes 3 split items | One item with 3 fulfillments |
| idRelacionado to track splits | Simple parent-child relationship |
| Status on each split item | Status derived from fulfillments |
| Hard to see "what did customer order?" | Order item is pristine record |
| Complex queries to aggregate | Simple aggregation |

### 2.2 Reservation vs Consumption

```
RESERVATION (soft claim)
├── Can expire after timeout
├── Can be cancelled anytime
├── Multiple reservations compete
└── FIFO order determines priority

CONSUMPTION (hard assignment)
├── Permanent (until return)
├── Creates fulfillment record
├── Decrements available stock
└── Links to specific order item
```

### 2.3 Events as Source of Truth

Instead of updating status fields, we record events:

```
Events for Order Item #1:
┌────────────────────────────────────────────────────────────────┐
│ 2024-01-01 10:00  OrderItemCreated      {qty: 10, price: 500}  │
│ 2024-01-05 14:30  StockReserved         {stock_id: 123, qty: 4}│
│ 2024-01-06 09:00  StockConsumed         {stock_id: 123, qty: 4}│
│ 2024-01-08 11:00  StockReserved         {stock_id: 456, qty: 4}│
│ 2024-01-09 15:00  StockConsumed         {stock_id: 456, qty: 4}│
│ 2024-01-10 08:00  DeliveryScheduled     {date: 2024-01-10, ...}│
│ 2024-01-10 14:00  ItemDelivered         {fulfillment_id: 1}    │
│ 2024-01-10 16:00  InvoiceGenerated      {nfe_id: 999}          │
└────────────────────────────────────────────────────────────────┘

Current state = replay(events)
```

---

## 3. Bounded Contexts

### 3.1 Context Map

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           ERP STACCATO                                   │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐              │
│  │    SALES     │───▶│  INVENTORY   │───▶│   DELIVERY   │              │
│  │   Context    │    │   Context    │    │   Context    │              │
│  └──────────────┘    └──────────────┘    └──────────────┘              │
│         │                   │                   │                       │
│         │                   │                   │                       │
│         ▼                   ▼                   ▼                       │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐              │
│  │   FISCAL     │◀───│  PURCHASING  │    │  FINANCIAL   │              │
│  │   Context    │    │   Context    │    │   Context    │              │
│  └──────────────┘    └──────────────┘    └──────────────┘              │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

### 3.2 Context Responsibilities

| Context | Owns | Publishes Events |
|---------|------|------------------|
| **Sales** | Orders, Items, Customers | OrderCreated, ItemAdded, OrderCancelled |
| **Inventory** | Stock, Reservations, Consumption | StockReceived, StockReserved, StockConsumed |
| **Purchasing** | Purchase Orders, Supplier Orders | POCreated, POReceived |
| **Delivery** | Schedules, Routes, Confirmations | DeliveryScheduled, DeliveryCompleted |
| **Fiscal** | NFe, Taxes | InvoiceGenerated, InvoiceAuthorized |
| **Financial** | Payments, Commissions | PaymentReceived, CommissionPaid |

### 3.3 Context Communication

Contexts communicate via **events**, not direct calls:

```php
// Sales context publishes
event(new OrderItemFulfilled($fulfillment));

// Delivery context listens
class ScheduleDeliveryOnFulfillment
{
    public function handle(OrderItemFulfilled $event)
    {
        // Auto-schedule delivery if all items ready
    }
}

// Fiscal context listens
class GenerateInvoiceOnDelivery
{
    public function handle(DeliveryCompleted $event)
    {
        // Generate NFe for delivered items
    }
}
```

---

## 4. The Order Lifecycle

### 4.1 Simplified Flow

```
┌────────────────────────────────────────────────────────────────────────┐
│                         ORDER LIFECYCLE                                 │
├────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌─────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐ │
│  │ CREATE  │───▶│   SOURCE    │───▶│   DELIVER   │───▶│   INVOICE   │ │
│  │  ORDER  │    │   ITEMS     │    │   ITEMS     │    │   ITEMS     │ │
│  └─────────┘    └─────────────┘    └─────────────┘    └─────────────┘ │
│       │               │                   │                  │         │
│       │               │                   │                  │         │
│       ▼               ▼                   ▼                  ▼         │
│   Customer        Find stock          Schedule           Generate      │
│   + Items         or order            delivery           NFe          │
│   + Prices        from supplier       + confirm          + payment    │
│                                                                         │
└────────────────────────────────────────────────────────────────────────┘
```

### 4.2 Order Entity

```php
class Order
{
    // Identity
    public int $id;
    public string $code;              // "V-2024-00001"

    // Relationships
    public int $customer_id;
    public ?int $seller_id;

    // Timing
    public Carbon $created_at;
    public ?Carbon $closed_at;

    // Derived status (calculated from items)
    public function status(): OrderStatus
    {
        $itemStatuses = $this->items->pluck('status');

        if ($itemStatuses->every(fn($s) => $s === ItemStatus::INVOICED)) {
            return OrderStatus::COMPLETED;
        }
        if ($itemStatuses->contains(ItemStatus::PENDING)) {
            return OrderStatus::IN_PROGRESS;
        }
        // ... etc
    }

    // Derived totals
    public function totalAmount(): Money
    {
        return $this->items->sum(fn($i) => $i->totalPrice());
    }
}
```

### 4.3 Order Item Entity

```php
class OrderItem
{
    // Identity
    public int $id;
    public int $order_id;

    // What was ordered (immutable after creation)
    public int $product_id;
    public int $quantity;
    public Money $unit_price;
    public Decimal $discount_percent;

    // Status derived from fulfillments
    public function status(): ItemStatus
    {
        $fulfilled = $this->fulfillments->sum('quantity');
        $delivered = $this->fulfillments
            ->where('status', FulfillmentStatus::DELIVERED)
            ->sum('quantity');
        $invoiced = $this->fulfillments
            ->where('status', FulfillmentStatus::INVOICED)
            ->sum('quantity');

        if ($invoiced === $this->quantity) return ItemStatus::INVOICED;
        if ($delivered === $this->quantity) return ItemStatus::DELIVERED;
        if ($fulfilled === $this->quantity) return ItemStatus::FULFILLED;
        if ($fulfilled > 0) return ItemStatus::PARTIALLY_FULFILLED;
        return ItemStatus::PENDING;
    }

    // How much still needs to be sourced?
    public function pendingQuantity(): int
    {
        return $this->quantity - $this->fulfillments->sum('quantity');
    }
}
```

### 4.4 Fulfillment Entity

```php
class Fulfillment
{
    // Identity
    public int $id;
    public int $order_item_id;

    // Source
    public int $stock_id;           // Which stock entry
    public int $quantity;           // How many units

    // Status
    public FulfillmentStatus $status;

    // Tracking
    public ?int $delivery_id;       // Which delivery
    public ?int $invoice_item_id;   // Which invoice line

    // Timestamps
    public Carbon $created_at;      // When reserved/consumed
    public ?Carbon $delivered_at;
    public ?Carbon $invoiced_at;
}

enum FulfillmentStatus: string
{
    case RESERVED = 'reserved';           // Stock claimed, not yet picked
    case READY = 'ready';                 // Picked, ready for delivery
    case IN_TRANSIT = 'in_transit';       // Out for delivery
    case DELIVERED = 'delivered';         // Customer received
    case INVOICED = 'invoiced';           // NFe generated
    case RETURNED = 'returned';           // Customer returned
}
```

---

## 5. Inventory Management

### 5.1 Stock Entry Entity

```php
class StockEntry
{
    // Identity
    public int $id;
    public int $product_id;
    public int $supplier_id;

    // Quantity tracking
    public int $quantity_received;      // Original amount
    public int $quantity_reserved;      // Soft claims (can expire)
    public int $quantity_consumed;      // Hard assignments

    // Derived
    public function quantityAvailable(): int
    {
        return $this->quantity_received
             - $this->quantity_reserved
             - $this->quantity_consumed;
    }

    // Source
    public ?int $purchase_order_item_id;
    public ?int $nfe_item_id;           // NFe that created this stock

    // FIFO ordering
    public Carbon $received_at;         // When stock arrived

    // Location
    public ?string $warehouse_block;    // "A-01", "B-03", etc.
}
```

### 5.2 The FIFO Algorithm

```php
class StockReservationService
{
    /**
     * Reserve stock for an order item using FIFO.
     * Returns list of reservations made.
     */
    public function reserveForItem(OrderItem $item): Collection
    {
        $needed = $item->pendingQuantity();
        $reservations = collect();

        // Find available stock, oldest first
        $stocks = StockEntry::query()
            ->where('product_id', $item->product_id)
            ->whereRaw('quantity_available() > 0')
            ->orderBy('received_at', 'asc')  // FIFO
            ->lockForUpdate()                 // Prevent race conditions
            ->get();

        foreach ($stocks as $stock) {
            if ($needed <= 0) break;

            $take = min($needed, $stock->quantityAvailable());

            // Create reservation
            $reservation = Reservation::create([
                'stock_entry_id' => $stock->id,
                'order_item_id' => $item->id,
                'quantity' => $take,
                'expires_at' => now()->addHours(24),
            ]);

            // Update stock counts
            $stock->increment('quantity_reserved', $take);

            $reservations->push($reservation);
            $needed -= $take;

            event(new StockReserved($reservation));
        }

        if ($needed > 0) {
            event(new InsufficientStock($item, $needed));
        }

        return $reservations;
    }
}
```

### 5.3 Reservation → Consumption

```php
class StockConsumptionService
{
    /**
     * Convert reservation to permanent consumption.
     * Called when order is confirmed / items picked.
     */
    public function consumeReservation(Reservation $reservation): Fulfillment
    {
        DB::transaction(function () use ($reservation, &$fulfillment) {
            $stock = $reservation->stockEntry;

            // Move from reserved to consumed
            $stock->decrement('quantity_reserved', $reservation->quantity);
            $stock->increment('quantity_consumed', $reservation->quantity);

            // Create fulfillment record
            $fulfillment = Fulfillment::create([
                'order_item_id' => $reservation->order_item_id,
                'stock_id' => $stock->id,
                'quantity' => $reservation->quantity,
                'status' => FulfillmentStatus::READY,
            ]);

            // Delete reservation
            $reservation->delete();

            event(new StockConsumed($fulfillment));
        });

        return $fulfillment;
    }
}
```

### 5.4 Stock Return

```php
class StockReturnService
{
    public function processReturn(Fulfillment $fulfillment, int $quantity, string $reason): StockReturn
    {
        return DB::transaction(function () use ($fulfillment, $quantity, $reason) {
            // Update fulfillment
            if ($quantity === $fulfillment->quantity) {
                $fulfillment->update(['status' => FulfillmentStatus::RETURNED]);
            } else {
                // Partial return - split fulfillment
                $fulfillment->decrement('quantity', $quantity);
                Fulfillment::create([
                    'order_item_id' => $fulfillment->order_item_id,
                    'stock_id' => $fulfillment->stock_id,
                    'quantity' => $quantity,
                    'status' => FulfillmentStatus::RETURNED,
                ]);
            }

            // Create return record
            $return = StockReturn::create([
                'fulfillment_id' => $fulfillment->id,
                'stock_entry_id' => $fulfillment->stock_id,
                'quantity' => $quantity,
                'reason' => $reason,
                'restock' => $this->canRestock($reason),
            ]);

            // If restockable, add back to available
            if ($return->restock) {
                $fulfillment->stockEntry->decrement('quantity_consumed', $quantity);
                event(new StockRestocked($return));
            }

            event(new ItemReturned($return));

            return $return;
        });
    }
}
```

---

## 6. Delivery Management

### 6.1 Delivery Entity

```php
class Delivery
{
    public int $id;
    public Carbon $scheduled_date;
    public ?Carbon $completed_at;

    public int $customer_id;
    public int $carrier_id;              // Transportadora
    public ?int $driver_id;

    public DeliveryStatus $status;
    public ?string $notes;

    // Address (snapshot at time of delivery)
    public Address $delivery_address;

    // Relationships
    public function fulfillments(): HasMany
    {
        return $this->hasMany(Fulfillment::class);
    }

    // Derived from fulfillments
    public function orders(): Collection
    {
        return $this->fulfillments
            ->map(fn($f) => $f->orderItem->order)
            ->unique('id');
    }
}

enum DeliveryStatus: string
{
    case SCHEDULED = 'scheduled';
    case LOADING = 'loading';
    case IN_TRANSIT = 'in_transit';
    case COMPLETED = 'completed';
    case FAILED = 'failed';           // Couldn't deliver
    case PARTIAL = 'partial';         // Some items refused
}
```

### 6.2 Delivery Scheduling Service

```php
class DeliverySchedulingService
{
    public function scheduleDelivery(
        Collection $fulfillments,
        Carbon $date,
        int $carrierId
    ): Delivery {
        // Validate all fulfillments are ready
        $notReady = $fulfillments->filter(
            fn($f) => $f->status !== FulfillmentStatus::READY
        );

        if ($notReady->isNotEmpty()) {
            throw new FulfillmentsNotReadyException($notReady);
        }

        // All fulfillments must be for same customer
        $customerIds = $fulfillments
            ->map(fn($f) => $f->orderItem->order->customer_id)
            ->unique();

        if ($customerIds->count() > 1) {
            throw new MultipleCustomersException();
        }

        return DB::transaction(function () use ($fulfillments, $date, $carrierId) {
            $customer = $fulfillments->first()->orderItem->order->customer;

            $delivery = Delivery::create([
                'scheduled_date' => $date,
                'customer_id' => $customer->id,
                'carrier_id' => $carrierId,
                'status' => DeliveryStatus::SCHEDULED,
                'delivery_address' => $customer->deliveryAddress(),
            ]);

            $fulfillments->each(function ($f) use ($delivery) {
                $f->update([
                    'delivery_id' => $delivery->id,
                    'status' => FulfillmentStatus::IN_TRANSIT,
                ]);
            });

            event(new DeliveryScheduled($delivery));

            return $delivery;
        });
    }
}
```

### 6.3 Delivery Confirmation

```php
class DeliveryConfirmationService
{
    public function confirmDelivery(
        Delivery $delivery,
        Collection $confirmedFulfillmentIds,
        ?string $notes = null
    ): void {
        DB::transaction(function () use ($delivery, $confirmedFulfillmentIds, $notes) {
            $allFulfillments = $delivery->fulfillments;
            $confirmed = $allFulfillments->whereIn('id', $confirmedFulfillmentIds);
            $refused = $allFulfillments->whereNotIn('id', $confirmedFulfillmentIds);

            // Mark confirmed items as delivered
            $confirmed->each(function ($f) {
                $f->update([
                    'status' => FulfillmentStatus::DELIVERED,
                    'delivered_at' => now(),
                ]);
                event(new ItemDelivered($f));
            });

            // Handle refused items
            $refused->each(function ($f) {
                $f->update(['status' => FulfillmentStatus::RETURNED]);
                event(new ItemRefused($f));
            });

            // Update delivery status
            $delivery->update([
                'status' => $refused->isEmpty()
                    ? DeliveryStatus::COMPLETED
                    : DeliveryStatus::PARTIAL,
                'completed_at' => now(),
                'notes' => $notes,
            ]);

            event(new DeliveryCompleted($delivery));
        });
    }
}
```

---

## 7. Fiscal & Financial

### 7.1 Invoice Generation

```php
class InvoiceService
{
    public function generateForDelivery(Delivery $delivery): Invoice
    {
        // Only invoice delivered items
        $deliveredFulfillments = $delivery->fulfillments
            ->where('status', FulfillmentStatus::DELIVERED);

        if ($deliveredFulfillments->isEmpty()) {
            throw new NoItemsToInvoiceException();
        }

        // Group by order for proper invoicing
        $byOrder = $deliveredFulfillments->groupBy(
            fn($f) => $f->orderItem->order_id
        );

        return DB::transaction(function () use ($byOrder, $delivery) {
            $invoice = Invoice::create([
                'customer_id' => $delivery->customer_id,
                'delivery_id' => $delivery->id,
                'status' => InvoiceStatus::DRAFT,
                'issue_date' => now(),
            ]);

            foreach ($byOrder as $orderId => $fulfillments) {
                foreach ($fulfillments as $fulfillment) {
                    $item = $fulfillment->orderItem;

                    $invoiceItem = $invoice->items()->create([
                        'product_id' => $item->product_id,
                        'quantity' => $fulfillment->quantity,
                        'unit_price' => $item->unit_price,
                        'discount_percent' => $item->discount_percent,
                        'fulfillment_id' => $fulfillment->id,
                    ]);

                    $fulfillment->update([
                        'invoice_item_id' => $invoiceItem->id,
                        'status' => FulfillmentStatus::INVOICED,
                        'invoiced_at' => now(),
                    ]);
                }
            }

            // Calculate taxes
            $this->taxCalculator->calculate($invoice);

            event(new InvoiceCreated($invoice));

            return $invoice;
        });
    }
}
```

### 7.2 NFe Integration

```php
interface NFeGateway
{
    public function transmit(Invoice $invoice): NFeResult;
    public function checkStatus(string $chave): NFeStatus;
    public function cancel(Invoice $invoice, string $justificativa): NFeResult;
}

class NFeService
{
    public function __construct(
        private NFeGateway $gateway,
        private NFeXmlBuilder $xmlBuilder
    ) {}

    public function authorize(Invoice $invoice): void
    {
        // Build XML
        $xml = $this->xmlBuilder->build($invoice);

        // Transmit to SEFAZ
        $result = $this->gateway->transmit($invoice);

        if ($result->isAuthorized()) {
            $invoice->update([
                'status' => InvoiceStatus::AUTHORIZED,
                'nfe_chave' => $result->chave,
                'nfe_protocolo' => $result->protocolo,
                'authorized_at' => now(),
            ]);

            event(new NFeAuthorized($invoice));

            // Generate payment slips
            $this->paymentService->generateBoletos($invoice);
        } else {
            $invoice->update([
                'status' => InvoiceStatus::REJECTED,
                'nfe_motivo_rejeicao' => $result->motivo,
            ]);

            event(new NFeRejected($invoice, $result->motivo));
        }
    }
}
```

### 7.3 Payment Tracking

```php
class Payment
{
    public int $id;
    public int $invoice_id;

    public PaymentMethod $method;       // BOLETO, PIX, CARTAO, etc.
    public Money $amount;
    public Carbon $due_date;
    public ?Carbon $paid_at;

    public PaymentStatus $status;

    // For boleto
    public ?string $boleto_linha_digitavel;
    public ?string $boleto_nosso_numero;
}

class PaymentService
{
    public function recordPayment(Payment $payment, Carbon $paidAt): void
    {
        $payment->update([
            'status' => PaymentStatus::PAID,
            'paid_at' => $paidAt,
        ]);

        event(new PaymentReceived($payment));

        // Check if invoice is fully paid
        $invoice = $payment->invoice;
        if ($invoice->isFullyPaid()) {
            event(new InvoiceFullyPaid($invoice));

            // Trigger commission calculation
            $this->commissionService->calculateFor($invoice);
        }
    }
}
```

### 7.4 Commission Calculation

```php
class CommissionService
{
    public function calculateFor(Invoice $invoice): Commission
    {
        // Only calculate after payment received
        if (!$invoice->isFullyPaid()) {
            throw new InvoiceNotPaidException();
        }

        $seller = $invoice->delivery->fulfillments
            ->first()
            ->orderItem
            ->order
            ->seller;

        // Get commission rate (from supplier or seller)
        $rate = $this->getCommissionRate($invoice);

        $commission = Commission::create([
            'invoice_id' => $invoice->id,
            'seller_id' => $seller->id,
            'base_amount' => $invoice->total_amount,
            'rate_percent' => $rate,
            'commission_amount' => $invoice->total_amount->multiply($rate / 100),
            'status' => CommissionStatus::PENDING,
        ]);

        event(new CommissionCalculated($commission));

        return $commission;
    }
}
```

---

## 8. State Machines

### 8.1 Order Item State Machine

```php
use Spatie\ModelStates\State;
use Spatie\ModelStates\StateConfig;

abstract class ItemState extends State
{
    abstract public function color(): string;
    abstract public function label(): string;
}

class Pending extends ItemState
{
    public function color(): string => 'gray';
    public function label(): string => 'Pendente';

    public function canTransitionTo(string $state): bool
    {
        return in_array($state, [
            PartiallyFulfilled::class,
            Fulfilled::class,
            Cancelled::class,
        ]);
    }
}

class PartiallyFulfilled extends ItemState
{
    public function color(): string => 'yellow';
    public function label(): string => 'Parcialmente Atendido';
}

class Fulfilled extends ItemState
{
    public function color(): string => 'blue';
    public function label(): string => 'Atendido';
}

class Delivered extends ItemState
{
    public function color(): string => 'green';
    public function label(): string => 'Entregue';
}

class Invoiced extends ItemState
{
    public function color(): string => 'emerald';
    public function label(): string => 'Faturado';

    // Final state - no transitions out
    public function canTransitionTo(string $state): bool
    {
        return false;
    }
}
```

### 8.2 State Machine Configuration

```php
class OrderItem extends Model
{
    protected $casts = [
        'status' => ItemState::class,
    ];

    public static function boot()
    {
        parent::boot();

        // Recalculate status when fulfillments change
        static::updated(function ($item) {
            $item->recalculateStatus();
        });
    }

    public function recalculateStatus(): void
    {
        $newStatus = $this->calculateStatusFromFulfillments();

        if ($this->status::class !== $newStatus) {
            $this->status->transitionTo($newStatus);
        }
    }

    private function calculateStatusFromFulfillments(): string
    {
        $fulfilled = $this->fulfillments->sum('quantity');
        $invoiced = $this->fulfillments
            ->where('status', FulfillmentStatus::INVOICED)
            ->sum('quantity');

        return match (true) {
            $invoiced >= $this->quantity => Invoiced::class,
            $this->fulfillments->every(fn($f) => $f->isDelivered()) => Delivered::class,
            $fulfilled >= $this->quantity => Fulfilled::class,
            $fulfilled > 0 => PartiallyFulfilled::class,
            default => Pending::class,
        };
    }
}
```

### 8.3 State Transition Guards

```php
class FulfillmentStateMachine
{
    public function canTransition(Fulfillment $f, FulfillmentStatus $to): bool
    {
        return match ([$f->status, $to]) {
            // From RESERVED
            [FulfillmentStatus::RESERVED, FulfillmentStatus::READY] => true,
            [FulfillmentStatus::RESERVED, FulfillmentStatus::RETURNED] => true,

            // From READY
            [FulfillmentStatus::READY, FulfillmentStatus::IN_TRANSIT] =>
                $f->delivery_id !== null,

            // From IN_TRANSIT
            [FulfillmentStatus::IN_TRANSIT, FulfillmentStatus::DELIVERED] => true,
            [FulfillmentStatus::IN_TRANSIT, FulfillmentStatus::RETURNED] => true,

            // From DELIVERED
            [FulfillmentStatus::DELIVERED, FulfillmentStatus::INVOICED] =>
                $f->invoice_item_id !== null,
            [FulfillmentStatus::DELIVERED, FulfillmentStatus::RETURNED] => true,

            // INVOICED is terminal
            [FulfillmentStatus::INVOICED, $_] => false,

            // RETURNED is terminal
            [FulfillmentStatus::RETURNED, $_] => false,

            default => false,
        };
    }

    public function transition(Fulfillment $f, FulfillmentStatus $to): void
    {
        if (!$this->canTransition($f, $to)) {
            throw new InvalidStateTransition($f->status, $to);
        }

        $from = $f->status;
        $f->update(['status' => $to]);

        event(new FulfillmentTransitioned($f, $from, $to));
    }
}
```

---

## 9. Event Architecture

### 9.1 Core Events

```php
// Sales Events
class OrderCreated { public Order $order; }
class OrderItemAdded { public OrderItem $item; }
class OrderCancelled { public Order $order; public string $reason; }

// Inventory Events
class StockReceived { public StockEntry $stock; }
class StockReserved { public Reservation $reservation; }
class StockConsumed { public Fulfillment $fulfillment; }
class StockRestocked { public StockReturn $return; }
class InsufficientStock { public OrderItem $item; public int $needed; }

// Delivery Events
class DeliveryScheduled { public Delivery $delivery; }
class DeliveryCompleted { public Delivery $delivery; }
class ItemDelivered { public Fulfillment $fulfillment; }
class ItemRefused { public Fulfillment $fulfillment; }

// Fiscal Events
class InvoiceCreated { public Invoice $invoice; }
class NFeAuthorized { public Invoice $invoice; }
class NFeRejected { public Invoice $invoice; public string $reason; }

// Financial Events
class PaymentReceived { public Payment $payment; }
class InvoiceFullyPaid { public Invoice $invoice; }
class CommissionCalculated { public Commission $commission; }
```

### 9.2 Event Listeners (Orchestration)

```php
// config/events.php
return [
    OrderCreated::class => [
        AutoReserveStockListener::class,
        NotifySellerListener::class,
    ],

    StockConsumed::class => [
        CheckOrderReadyForDeliveryListener::class,
    ],

    DeliveryCompleted::class => [
        AutoGenerateInvoiceListener::class,
        UpdateOrderStatusListener::class,
    ],

    NFeAuthorized::class => [
        GenerateBoletosListener::class,
        SendInvoiceToCustomerListener::class,
    ],

    PaymentReceived::class => [
        CalculateCommissionListener::class,
        UpdateInvoiceStatusListener::class,
    ],
];
```

### 9.3 Event Store (Audit Trail)

```php
class EventStore extends Model
{
    protected $table = 'event_store';

    protected $casts = [
        'payload' => 'array',
        'metadata' => 'array',
    ];
}

class StoreEventListener
{
    public function handle(object $event): void
    {
        EventStore::create([
            'event_type' => get_class($event),
            'aggregate_type' => $this->getAggregateType($event),
            'aggregate_id' => $this->getAggregateId($event),
            'payload' => $this->serialize($event),
            'metadata' => [
                'user_id' => auth()->id(),
                'ip_address' => request()->ip(),
                'user_agent' => request()->userAgent(),
            ],
            'occurred_at' => now(),
        ]);
    }
}

// Register for all events
Event::listen('*', StoreEventListener::class);
```

---

## 10. Database Schema

### 10.1 Core Tables

```sql
-- ENUMS
CREATE TYPE order_status AS ENUM ('draft', 'confirmed', 'in_progress', 'completed', 'cancelled');
CREATE TYPE item_status AS ENUM ('pending', 'partially_fulfilled', 'fulfilled', 'delivered', 'invoiced', 'cancelled');
CREATE TYPE fulfillment_status AS ENUM ('reserved', 'ready', 'in_transit', 'delivered', 'invoiced', 'returned');
CREATE TYPE delivery_status AS ENUM ('scheduled', 'loading', 'in_transit', 'completed', 'failed', 'partial');
CREATE TYPE invoice_status AS ENUM ('draft', 'pending', 'authorized', 'rejected', 'cancelled');
CREATE TYPE payment_status AS ENUM ('pending', 'paid', 'overdue', 'cancelled');

-- ORDERS
CREATE TABLE orders (
    id SERIAL PRIMARY KEY,
    code VARCHAR(20) UNIQUE NOT NULL,
    customer_id INTEGER NOT NULL REFERENCES customers(id),
    seller_id INTEGER REFERENCES users(id),
    status order_status NOT NULL DEFAULT 'draft',
    notes TEXT,
    created_at TIMESTAMP NOT NULL DEFAULT NOW(),
    confirmed_at TIMESTAMP,
    closed_at TIMESTAMP
);

CREATE TABLE order_items (
    id SERIAL PRIMARY KEY,
    order_id INTEGER NOT NULL REFERENCES orders(id),
    product_id INTEGER NOT NULL REFERENCES products(id),
    quantity INTEGER NOT NULL CHECK (quantity > 0),
    unit_price DECIMAL(10,2) NOT NULL,
    discount_percent DECIMAL(5,2) NOT NULL DEFAULT 0,
    status item_status NOT NULL DEFAULT 'pending',
    created_at TIMESTAMP NOT NULL DEFAULT NOW()
);

-- INVENTORY
CREATE TABLE stock_entries (
    id SERIAL PRIMARY KEY,
    product_id INTEGER NOT NULL REFERENCES products(id),
    supplier_id INTEGER NOT NULL REFERENCES suppliers(id),
    quantity_received INTEGER NOT NULL,
    quantity_reserved INTEGER NOT NULL DEFAULT 0,
    quantity_consumed INTEGER NOT NULL DEFAULT 0,
    unit_cost DECIMAL(10,2),
    purchase_order_item_id INTEGER REFERENCES purchase_order_items(id),
    nfe_item_id INTEGER REFERENCES nfe_items(id),
    warehouse_block VARCHAR(10),
    received_at TIMESTAMP NOT NULL DEFAULT NOW(),

    -- Computed available quantity
    CONSTRAINT valid_quantities CHECK (
        quantity_reserved >= 0 AND
        quantity_consumed >= 0 AND
        quantity_received >= quantity_reserved + quantity_consumed
    )
);

CREATE TABLE reservations (
    id SERIAL PRIMARY KEY,
    stock_entry_id INTEGER NOT NULL REFERENCES stock_entries(id),
    order_item_id INTEGER NOT NULL REFERENCES order_items(id),
    quantity INTEGER NOT NULL CHECK (quantity > 0),
    expires_at TIMESTAMP NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT NOW()
);

CREATE TABLE fulfillments (
    id SERIAL PRIMARY KEY,
    order_item_id INTEGER NOT NULL REFERENCES order_items(id),
    stock_entry_id INTEGER NOT NULL REFERENCES stock_entries(id),
    quantity INTEGER NOT NULL CHECK (quantity > 0),
    status fulfillment_status NOT NULL DEFAULT 'reserved',
    delivery_id INTEGER REFERENCES deliveries(id),
    invoice_item_id INTEGER REFERENCES invoice_items(id),
    created_at TIMESTAMP NOT NULL DEFAULT NOW(),
    delivered_at TIMESTAMP,
    invoiced_at TIMESTAMP
);

-- DELIVERIES
CREATE TABLE deliveries (
    id SERIAL PRIMARY KEY,
    customer_id INTEGER NOT NULL REFERENCES customers(id),
    carrier_id INTEGER NOT NULL REFERENCES carriers(id),
    driver_id INTEGER REFERENCES users(id),
    scheduled_date DATE NOT NULL,
    status delivery_status NOT NULL DEFAULT 'scheduled',

    -- Address snapshot
    delivery_street VARCHAR(200) NOT NULL,
    delivery_number VARCHAR(20),
    delivery_complement VARCHAR(100),
    delivery_neighborhood VARCHAR(100),
    delivery_city VARCHAR(100) NOT NULL,
    delivery_state CHAR(2) NOT NULL,
    delivery_zip VARCHAR(9) NOT NULL,

    notes TEXT,
    created_at TIMESTAMP NOT NULL DEFAULT NOW(),
    completed_at TIMESTAMP
);

-- INVOICES
CREATE TABLE invoices (
    id SERIAL PRIMARY KEY,
    customer_id INTEGER NOT NULL REFERENCES customers(id),
    delivery_id INTEGER REFERENCES deliveries(id),
    status invoice_status NOT NULL DEFAULT 'draft',

    -- NFe data
    nfe_numero INTEGER,
    nfe_serie INTEGER,
    nfe_chave VARCHAR(44) UNIQUE,
    nfe_protocolo VARCHAR(20),
    nfe_xml TEXT,

    -- Totals
    total_produtos DECIMAL(10,2) NOT NULL DEFAULT 0,
    total_frete DECIMAL(10,2) NOT NULL DEFAULT 0,
    total_desconto DECIMAL(10,2) NOT NULL DEFAULT 0,
    total_impostos DECIMAL(10,2) NOT NULL DEFAULT 0,
    total_nota DECIMAL(10,2) NOT NULL DEFAULT 0,

    issue_date DATE,
    authorized_at TIMESTAMP,
    created_at TIMESTAMP NOT NULL DEFAULT NOW()
);

CREATE TABLE invoice_items (
    id SERIAL PRIMARY KEY,
    invoice_id INTEGER NOT NULL REFERENCES invoices(id),
    product_id INTEGER NOT NULL REFERENCES products(id),
    fulfillment_id INTEGER NOT NULL REFERENCES fulfillments(id),
    quantity INTEGER NOT NULL,
    unit_price DECIMAL(10,2) NOT NULL,
    discount_percent DECIMAL(5,2) NOT NULL DEFAULT 0,
    total DECIMAL(10,2) NOT NULL
);

-- PAYMENTS
CREATE TABLE payments (
    id SERIAL PRIMARY KEY,
    invoice_id INTEGER NOT NULL REFERENCES invoices(id),
    method VARCHAR(20) NOT NULL,
    amount DECIMAL(10,2) NOT NULL,
    due_date DATE NOT NULL,
    status payment_status NOT NULL DEFAULT 'pending',

    -- Boleto data
    boleto_linha_digitavel VARCHAR(47),
    boleto_nosso_numero VARCHAR(20),
    boleto_pdf_path VARCHAR(255),

    paid_at TIMESTAMP,
    created_at TIMESTAMP NOT NULL DEFAULT NOW()
);

-- EVENT STORE
CREATE TABLE event_store (
    id BIGSERIAL PRIMARY KEY,
    event_type VARCHAR(100) NOT NULL,
    aggregate_type VARCHAR(50) NOT NULL,
    aggregate_id INTEGER NOT NULL,
    payload JSONB NOT NULL,
    metadata JSONB,
    occurred_at TIMESTAMP NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_events_aggregate ON event_store(aggregate_type, aggregate_id);
CREATE INDEX idx_events_type ON event_store(event_type);
CREATE INDEX idx_events_occurred ON event_store(occurred_at);
```

### 10.2 Useful Views

```sql
-- Order items with fulfillment summary
CREATE VIEW v_order_items_summary AS
SELECT
    oi.id,
    oi.order_id,
    oi.product_id,
    p.description as product_name,
    oi.quantity as ordered_qty,
    COALESCE(SUM(f.quantity) FILTER (WHERE f.status != 'returned'), 0) as fulfilled_qty,
    COALESCE(SUM(f.quantity) FILTER (WHERE f.status = 'delivered'), 0) as delivered_qty,
    COALESCE(SUM(f.quantity) FILTER (WHERE f.status = 'invoiced'), 0) as invoiced_qty,
    oi.quantity - COALESCE(SUM(f.quantity) FILTER (WHERE f.status != 'returned'), 0) as pending_qty,
    oi.status
FROM order_items oi
JOIN products p ON p.id = oi.product_id
LEFT JOIN fulfillments f ON f.order_item_id = oi.id
GROUP BY oi.id, p.description;

-- Available stock by product (FIFO order)
CREATE VIEW v_stock_available AS
SELECT
    se.id as stock_entry_id,
    se.product_id,
    p.description as product_name,
    s.razao_social as supplier_name,
    se.quantity_received - se.quantity_reserved - se.quantity_consumed as available,
    se.received_at,
    se.warehouse_block
FROM stock_entries se
JOIN products p ON p.id = se.product_id
JOIN suppliers s ON s.id = se.supplier_id
WHERE se.quantity_received > se.quantity_reserved + se.quantity_consumed
ORDER BY se.product_id, se.received_at;
```

---

## 11. Testing Strategy

### 11.1 Unit Tests (Pure Business Logic)

```php
class StockReservationTest extends TestCase
{
    /** @test */
    public function it_reserves_stock_using_fifo()
    {
        // Arrange
        $oldStock = StockEntry::factory()->create([
            'quantity_received' => 5,
            'received_at' => now()->subDays(10),
        ]);
        $newStock = StockEntry::factory()->create([
            'quantity_received' => 5,
            'product_id' => $oldStock->product_id,
            'received_at' => now()->subDays(1),
        ]);

        $item = OrderItem::factory()->create([
            'product_id' => $oldStock->product_id,
            'quantity' => 7,
        ]);

        // Act
        $service = new StockReservationService();
        $reservations = $service->reserveForItem($item);

        // Assert
        $this->assertCount(2, $reservations);
        $this->assertEquals(5, $reservations[0]->quantity); // All of old stock
        $this->assertEquals(2, $reservations[1]->quantity); // Part of new stock
        $this->assertEquals($oldStock->id, $reservations[0]->stock_entry_id);
    }

    /** @test */
    public function it_fires_insufficient_stock_event_when_not_enough()
    {
        Event::fake([InsufficientStock::class]);

        $stock = StockEntry::factory()->create(['quantity_received' => 3]);
        $item = OrderItem::factory()->create([
            'product_id' => $stock->product_id,
            'quantity' => 10,
        ]);

        $service = new StockReservationService();
        $reservations = $service->reserveForItem($item);

        $this->assertEquals(3, $reservations->sum('quantity'));
        Event::assertDispatched(InsufficientStock::class, function ($e) {
            return $e->needed === 7;
        });
    }
}
```

### 11.2 State Machine Tests

```php
class FulfillmentStateMachineTest extends TestCase
{
    /** @test */
    public function reserved_can_transition_to_ready()
    {
        $fulfillment = Fulfillment::factory()->create([
            'status' => FulfillmentStatus::RESERVED,
        ]);

        $sm = new FulfillmentStateMachine();

        $this->assertTrue($sm->canTransition($fulfillment, FulfillmentStatus::READY));
    }

    /** @test */
    public function in_transit_cannot_transition_to_ready()
    {
        $fulfillment = Fulfillment::factory()->create([
            'status' => FulfillmentStatus::IN_TRANSIT,
        ]);

        $sm = new FulfillmentStateMachine();

        $this->assertFalse($sm->canTransition($fulfillment, FulfillmentStatus::READY));
    }

    /** @test */
    public function invoiced_is_terminal_state()
    {
        $fulfillment = Fulfillment::factory()->create([
            'status' => FulfillmentStatus::INVOICED,
        ]);

        $sm = new FulfillmentStateMachine();

        foreach (FulfillmentStatus::cases() as $status) {
            $this->assertFalse($sm->canTransition($fulfillment, $status));
        }
    }
}
```

### 11.3 Integration Tests (Full Flows)

```php
class OrderToInvoiceFlowTest extends TestCase
{
    use RefreshDatabase;

    /** @test */
    public function complete_order_flow()
    {
        // Setup
        $customer = Customer::factory()->create();
        $product = Product::factory()->create();
        $supplier = Supplier::factory()->create();
        $carrier = Carrier::factory()->create();

        // Create stock
        $stock = StockEntry::factory()->create([
            'product_id' => $product->id,
            'supplier_id' => $supplier->id,
            'quantity_received' => 10,
        ]);

        // 1. Create order
        $order = Order::create([
            'customer_id' => $customer->id,
            'status' => OrderStatus::CONFIRMED,
        ]);

        $item = $order->items()->create([
            'product_id' => $product->id,
            'quantity' => 5,
            'unit_price' => 100.00,
        ]);

        // 2. Reserve stock
        $reservationService = app(StockReservationService::class);
        $reservations = $reservationService->reserveForItem($item);

        $this->assertEquals(5, $reservations->sum('quantity'));
        $this->assertEquals(5, $stock->fresh()->quantity_reserved);

        // 3. Consume stock
        $consumptionService = app(StockConsumptionService::class);
        $fulfillment = $consumptionService->consumeReservation($reservations->first());

        $this->assertEquals(FulfillmentStatus::READY, $fulfillment->status);
        $this->assertEquals(5, $stock->fresh()->quantity_consumed);
        $this->assertEquals(0, $stock->fresh()->quantity_reserved);

        // 4. Schedule delivery
        $deliveryService = app(DeliverySchedulingService::class);
        $delivery = $deliveryService->scheduleDelivery(
            collect([$fulfillment]),
            now()->addDay(),
            $carrier->id
        );

        $this->assertEquals(DeliveryStatus::SCHEDULED, $delivery->status);
        $this->assertEquals(FulfillmentStatus::IN_TRANSIT, $fulfillment->fresh()->status);

        // 5. Confirm delivery
        $confirmationService = app(DeliveryConfirmationService::class);
        $confirmationService->confirmDelivery($delivery, [$fulfillment->id]);

        $this->assertEquals(DeliveryStatus::COMPLETED, $delivery->fresh()->status);
        $this->assertEquals(FulfillmentStatus::DELIVERED, $fulfillment->fresh()->status);

        // 6. Generate invoice
        $invoiceService = app(InvoiceService::class);
        $invoice = $invoiceService->generateForDelivery($delivery->fresh());

        $this->assertEquals(InvoiceStatus::DRAFT, $invoice->status);
        $this->assertEquals(500.00, $invoice->total_produtos);
        $this->assertEquals(FulfillmentStatus::INVOICED, $fulfillment->fresh()->status);

        // 7. Verify order status
        $this->assertEquals(ItemStatus::INVOICED, $item->fresh()->status());
        $this->assertEquals(OrderStatus::COMPLETED, $order->fresh()->status());
    }
}
```

### 11.4 Event-Driven Tests

```php
class EventFlowTest extends TestCase
{
    /** @test */
    public function delivery_completion_triggers_invoice_generation()
    {
        Event::fake([InvoiceCreated::class]);

        $delivery = Delivery::factory()
            ->has(Fulfillment::factory()->count(3)->state([
                'status' => FulfillmentStatus::IN_TRANSIT,
            ]))
            ->create();

        // Simulate delivery completion
        $service = app(DeliveryConfirmationService::class);
        $service->confirmDelivery(
            $delivery,
            $delivery->fulfillments->pluck('id')->toArray()
        );

        // AutoGenerateInvoiceListener should fire
        Event::assertDispatched(InvoiceCreated::class);
    }
}
```

---

## 12. Advanced Features

### 12.1 Partial Delivery Handling

```php
class PartialDeliveryService
{
    public function handlePartialDelivery(
        Delivery $delivery,
        array $deliveredFulfillmentIds,
        string $reason
    ): void {
        DB::transaction(function () use ($delivery, $deliveredFulfillmentIds, $reason) {
            $allFulfillments = $delivery->fulfillments;
            $delivered = $allFulfillments->whereIn('id', $deliveredFulfillmentIds);
            $refused = $allFulfillments->whereNotIn('id', $deliveredFulfillmentIds);

            // Mark delivered items
            $delivered->each(fn($f) => $f->update([
                'status' => FulfillmentStatus::DELIVERED,
                'delivered_at' => now(),
            ]));

            // Handle refused items - create new delivery attempt
            if ($refused->isNotEmpty()) {
                $newDelivery = Delivery::create([
                    'customer_id' => $delivery->customer_id,
                    'carrier_id' => $delivery->carrier_id,
                    'scheduled_date' => now()->addDays(3),
                    'status' => DeliveryStatus::SCHEDULED,
                    'notes' => "Retry: {$reason}",
                ]);

                $refused->each(fn($f) => $f->update([
                    'delivery_id' => $newDelivery->id,
                    'status' => FulfillmentStatus::READY, // Reset to ready
                ]));
            }

            $delivery->update([
                'status' => DeliveryStatus::PARTIAL,
                'completed_at' => now(),
                'notes' => $reason,
            ]);
        });
    }
}
```

### 12.2 Multi-Supplier Same Product

```php
// The fulfillment model naturally handles this
// One order item can have fulfillments from different stock entries (suppliers)

$item = OrderItem::find(1);

// Fulfillments show the source
$item->fulfillments->each(function ($f) {
    echo "{$f->quantity} from {$f->stockEntry->supplier->name}";
});

// Output:
// 3 from Supplier A
// 2 from Supplier B
// 5 from Supplier C
```

### 12.3 Backorder Management

```php
class BackorderService
{
    public function handleInsufficientStock(InsufficientStock $event): void
    {
        // Find or create purchase order for this product
        $po = PurchaseOrder::firstOrCreate(
            [
                'supplier_id' => $event->item->product->default_supplier_id,
                'status' => POStatus::DRAFT,
            ],
            [
                'expected_date' => now()->addDays(14),
            ]
        );

        // Add item to PO
        $po->items()->create([
            'product_id' => $event->item->product_id,
            'quantity' => $event->needed,
            'linked_order_item_id' => $event->item->id, // Track relationship
        ]);

        // Mark order item as backordered
        Backorder::create([
            'order_item_id' => $event->item->id,
            'quantity' => $event->needed,
            'purchase_order_id' => $po->id,
            'expected_date' => $po->expected_date,
        ]);
    }
}

// When stock arrives, auto-fulfill backorders
class FulfillBackordersOnStockReceived
{
    public function handle(StockReceived $event): void
    {
        $backorders = Backorder::query()
            ->whereHas('orderItem', fn($q) =>
                $q->where('product_id', $event->stock->product_id)
            )
            ->where('status', 'pending')
            ->orderBy('created_at') // FIFO for backorders too
            ->get();

        foreach ($backorders as $backorder) {
            $this->reservationService->reserveForItem($backorder->orderItem);
            $backorder->update(['status' => 'fulfilled']);
        }
    }
}
```

### 12.4 Price Change Tracking

```php
// Prices are immutable on order items
// If price changes needed, create adjustment record

class PriceAdjustment
{
    public int $id;
    public int $order_item_id;
    public Money $original_price;
    public Money $adjusted_price;
    public string $reason;
    public int $approved_by;
    public Carbon $created_at;
}

// The order item's effective price considers adjustments
class OrderItem
{
    public function effectiveUnitPrice(): Money
    {
        $adjustment = $this->priceAdjustment;
        return $adjustment ? $adjustment->adjusted_price : $this->unit_price;
    }
}
```

### 12.5 Audit Trail Queries

```php
// What happened to this order?
$events = EventStore::query()
    ->where('aggregate_type', 'order')
    ->where('aggregate_id', $orderId)
    ->orderBy('occurred_at')
    ->get();

// Who changed this item's status?
$transitions = EventStore::query()
    ->where('event_type', FulfillmentTransitioned::class)
    ->whereJsonContains('payload->fulfillment_id', $fulfillmentId)
    ->get();

// Replay order state at specific point in time
class OrderStateRebuilder
{
    public function rebuildAt(int $orderId, Carbon $pointInTime): array
    {
        $events = EventStore::query()
            ->where('aggregate_type', 'order')
            ->where('aggregate_id', $orderId)
            ->where('occurred_at', '<=', $pointInTime)
            ->orderBy('occurred_at')
            ->get();

        return $this->applyEvents($events);
    }
}
```

---

## Summary: Key Differences from Current System

| Aspect | Current System | Greenfield Design |
|--------|----------------|-------------------|
| **Item Tracking** | L1/L2 split tables | Single item + fulfillments |
| **Stock Assignment** | idEstoque on produto | FIFO reservation → consumption |
| **Status** | Magic strings | Enums with state machines |
| **Supplier Ref** | VARCHAR copied | FK everywhere |
| **Audit** | None | Event sourcing |
| **Partial Delivery** | Split items | Fulfillment records |
| **Business Logic** | Scattered in UI | Domain services |
| **Testing** | Minimal | Comprehensive, easy to test |
| **Concurrency** | Race conditions | Pessimistic locking (FOR UPDATE) |

The key insight: **Fulfillments are the core concept**, not split items. Everything flows from reservation → consumption → delivery → invoice.
