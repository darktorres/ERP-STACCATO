# Staccato ERP - Comprehensive Redesign Document 2025

## 📋 Document Index

### **1. [Executive Summary](#executive-summary)** (Line 150)
- Comprehensive analysis of current system and redesign proposal
- Current system: Qt C++ desktop application, 290+ source files, ~100,000+ lines
- Database: MySQL with 209 tables and 136 views
- Key problems: Database complexity, monolithic architecture, tight coupling

### **2. [Current System Analysis](#current-system-analysis)** (Line 168)

#### **2.1 Database Architecture Issues** (Lines 23-64)
- **Over-Complex Database Design** (Lines 25-41)
  - 209 Tables with unclear relationships
  - 136 Views with embedded business logic
  - Circular dependencies and performance issues
  - Evidence from initdb.sql
- **Database Normalization Problems** (Lines 42-64)
  - Mixed concerns in single tables
  - Redundant data and inconsistent naming
  - Missing constraints example: `loja` table analysis

#### **2.2 Application Architecture Issues** (Lines 65-114)
- **Monolithic God Class** (Lines 67-100)
  - Application class violating Single Responsibility Principle
  - Database management + transaction management + business operations
  - UI management and utility functions in single class
- **Scattered Business Logic** (Lines 101-107)
  - Business logic distributed across database views and application code
- **Tight UI-Business Coupling** (Lines 108-114)
  - Qt widgets directly coupled to database operations

#### **2.3 Code Organization Issues** (Lines 115-133)
- **Evidence from TODOs in main.cpp** (Lines 117-133)
  - Unresolved technical debt indicators
  - Inconsistent initialization patterns

#### **2.4 Organic Growth Problems** (Lines 134-148)
- **Inconsistent Patterns** (Lines 136-141)
  - Multiple approaches to similar problems
- **Module Boundaries** (Lines 142-148)
  - Unclear separation of concerns

#### **2.5 Performance and Scalability Issues** (Lines 149-160)
- **Database Performance Problems** (Lines 151-155)
  - Complex view queries causing slow performance
- **Application Performance Issues** (Lines 156-160)
  - Desktop-only limitation

#### **2.6 Maintenance and Development Issues** (Lines 161-172)
- **Testing Challenges** (Lines 163-167)
  - Tight coupling makes unit testing difficult
- **Development Velocity Issues** (Lines 168-172)
  - Time-consuming changes and high bug risk

### **3. [Proposed Modern Architecture](#proposed-modern-architecture)** (Line 320)

#### **3.1 Clean Architecture Design** (Lines 175-213)
- **Hexagonal Architecture (Ports and Adapters)** (Lines 177-213)
  - Domain layer isolation
  - Infrastructure abstraction
  - Mermaid diagram of clean architecture

#### **3.2 Domain-Driven Design (DDD) Approach** (Lines 214-316)
- **Identified Bounded Contexts** (Lines 216-316)
  - **Sales Context**: Customer management, order processing, quotations
  - **Inventory Context**: Product catalog, stock management, warehouse operations
  - **Purchasing Context**: Supplier management, purchase orders, receiving
  - **Financial Context**: Accounts receivable/payable, invoicing, payments
  - **Brazilian Compliance Context**: NFe, tax calculations, regulatory reporting
  - **Logistics Context**: Shipping, delivery, route optimization
  - **Reporting Context**: Business intelligence, analytics, report generation

#### **3.3 Redesigned Database Schema** (Lines 317-642)
- **Core Entities with Proper Separation** (Lines 319-642)
  - Complete normalized schema design
  - Proper foreign key relationships
  - Audit fields and temporal tracking
  - Brazilian compliance integration

#### **3.4 Modern Application Architecture** (Lines 643-860)
- **Microservices Architecture** (Lines 645-699)
  - Service decomposition strategy
  - Inter-service communication patterns
  - Database-per-service principle
- **Domain Events and CQRS** (Lines 700-860)
  - Event-driven architecture
  - Command Query Responsibility Segregation
  - Event sourcing for audit trails

#### **3.5 API Design** (Lines 861-1024)
- **REST API Design** (Lines 863-1024)
  - Resource-based endpoints
  - Consistent error handling
  - Authentication and authorization
  - API versioning strategy
  - Comprehensive endpoint examples

#### **3.6 Frontend Architecture** (Lines 1025-1250)
- **Modern React Application Structure** (Lines 1027-1250)
  - Component architecture
  - State management with Redux Toolkit
  - Routing and navigation
  - UI component library integration
  - Performance optimization strategies

#### **3.7 Testing Strategy** (Lines 1251-1529)
- **Comprehensive Testing Approach** (Lines 1253-1529)
  - Unit testing frameworks
  - Integration testing strategies
  - End-to-end testing
  - Performance testing
  - Test automation pipeline

#### **3.8 Migration Strategy** (Lines 1530-1677)
- **Phased Migration Approach** (Lines 1532-1677)
  - **Phase 1: Foundation** (1-3 months): Database redesign, basic API
  - **Phase 2: Core Services** (4-6 months): Essential business logic migration
  - **Phase 3: Frontend Development** (7-9 months): React interface implementation
  - **Phase 4: Advanced Features** (10-12 months): Reports, analytics, optimization

### **4. Implementation Timeline and Resource Requirements** (Lines 1678-1717)
- **Timeline: 12-Month Migration** (Lines 1680-1697)
  - Month-by-month breakdown
  - Deliverables and milestones
- **Resource Requirements** (Lines 1698-1717)
  - Development team: 11 specialists
  - Infrastructure requirements
  - Total estimated cost: $800K-1.2M

### **5. Benefits and Expected Outcomes** (Lines 1718-1740)
- **Technical Benefits** (Lines 1720-1726)
  - Improved maintainability, performance, scalability
  - Enhanced testability and developer experience
- **Business Benefits** (Lines 1727-1733)
  - Reduced development costs, improved UX
  - Better business insights and compliance
- **Risk Mitigation** (Lines 1734-1740)
  - Gradual migration, parallel running
  - Comprehensive testing and rollback procedures

### **6. Conclusion** (Lines 1741-1753)
- Summary of architectural redesign benefits
- Investment justification: $800K-1.2M for 10+ year future-proof system
- Transformation from organic growth to well-architected solution

---

## Executive Summary

This document provides a comprehensive analysis of the current Staccato ERP system and proposes a complete architectural redesign to address organic growth issues, improve maintainability, and modernize the technology stack. The current system, while functional, suffers from significant architectural debt that hinders scalability, maintainability, and development velocity.

**Current System Overview:**
- **Technology**: Qt C++ desktop application
- **Database**: MySQL with 209 tables and 136 views
- **Codebase Size**: 290+ source files, ~100,000+ lines of code
- **Architecture**: Monolithic desktop application with business logic scattered across database views and application code

**Key Problems Identified:**
1. Excessive database complexity (136 views with embedded business logic)
2. Monolithic architecture limiting scalability
3. Tight coupling between UI and business logic
4. Lack of proper separation of concerns
5. Organic growth leading to inconsistent patterns
6. Limited testability and maintainability

## Current System Analysis

### 1. Database Architecture Issues

#### **Problem: Over-Complex Database Design**
- **209 Tables**: Excessive number of tables with unclear relationships
- **136 Views**: Critical business logic embedded in database views
- **Circular Dependencies**: Views referencing other views creating maintenance nightmares
- **Performance Issues**: Complex views causing slow queries

**Evidence from initdb.sql:**
```sql
-- Example of overly complex view with business logic
CREATE VIEW `EDU_view_Vendas` AS
select `vp`.`idVendaProduto2`,
       date_format(`v`.`data`,'%Y/%m') AS `mes`,
       -- 50+ complex calculated fields with nested conditions
       if((`v`.`representacao` = 1),0,((`v`.`frete` / (`v`.`total` - `v`.`frete`)) * `vp`.`total`)) AS `frete`,
       -- More complex business calculations...
```

#### **Database Normalization Problems**
- **Mixed Concerns**: Single tables handling multiple business domains
- **Redundant Data**: Repeated information across multiple tables
- **Inconsistent Naming**: Portuguese and English mixed throughout schema
- **Missing Constraints**: Limited foreign key constraints and validation

**Examples of Problematic Tables:**
```sql
-- loja table mixing store info with logistics and NFe configuration
CREATE TABLE `loja` (
  `idLoja` INT(10) UNSIGNED NOT NULL AUTO_INCREMENT,
  `descricao` VARCHAR(45) NULL,
  `nomeFantasia` VARCHAR(45) NOT NULL,
  -- ... store info ...
  `porcentagemFrete` DECIMAL(15,4) NOT NULL DEFAULT '4.0000',
  `valorMinimoFrete` DECIMAL(15,4) NOT NULL DEFAULT '80.0000',
  -- ... logistics info ...
  `certificadoSerie` VARCHAR(32) NULL,
  `certificadoSenha` VARCHAR(45) NULL,
  -- ... NFe certificate info mixed in ...
)
```

### 2. Application Architecture Issues

#### **Problem: Monolithic God Class**
The `Application` class violates Single Responsibility Principle:

```cpp
class Application final : public QApplication {
  // Database management
  auto dbConnect(const QString &hostname, const QString &user, const QString &userPassword) -> void;
  auto dbReconnect(const bool isSilent = false) -> void;

  // Transaction management
  auto startTransaction(const QString &messageLog) -> void;
  auto endTransaction() -> void;
  auto rollbackTransaction(const QString &message) -> void;

  // Business operations
  auto abrirCompra(const QVariant &ordemCompra) -> void;
  auto abrirEstoque(const QVariant &idEstoque) -> void;
  auto abrirNFe(const QVariant &idNFe) -> void;
  auto abrirVenda(const QVariant &idVenda) -> void;

  // Utility functions
  auto roundDouble(const double value) -> double;
  auto sanitizeSQL(const QString &string) -> QString;
  auto removerDiacriticos(const QString &s, const bool removerSimbolos = false) -> QString;

  // UI management
  auto enqueueError(const QString &error, QWidget *parent = nullptr) -> void;
  auto darkTheme() -> void;
  auto lightTheme() -> void;

  // And many more responsibilities...
};
```

#### **Problem: Scattered Business Logic**
Business rules are distributed across:
1. Database views (136 complex views)
2. Application code (scattered across widgets)
3. SQL queries embedded in UI components
4. Stored procedures and database functions

#### **Problem: Tight UI-Business Coupling**
From analysis of widget classes:
- UI widgets directly execute SQL queries
- Business calculations embedded in UI components
- No clear separation between presentation and business logic
- Direct database access from UI components

### 3. Code Organization Issues

#### **Evidence from TODOs in main.cpp:**
```cpp
// TODO: replace insert querys with model so values can be correctly rounded (Application::roundDouble)
// TODO: test changing table header to resizeToContents
// TODO: evitar divisoes por zero
// TODO: criar um delegate unidade para concatenar a unidade na coluna quant?
// TODO: divide views into categories like: view_compra_..., view_logistica_..., view_financeiro_..., etc
// TODO: use initializer lists?
```

These TODOs reveal:
1. **Data Integrity Issues**: Manual rounding suggests lack of proper data models
2. **UI Inconsistencies**: Header resizing issues indicate UI architecture problems
3. **Error-Prone Calculations**: Division by zero suggests lack of validation
4. **Disorganized Database Views**: Views need categorization
5. **Code Quality Issues**: Modern C++ practices not consistently applied

### 4. Organic Growth Problems

#### **Inconsistent Patterns**
- **Mixed Naming Conventions**: `cliente`, `usuario` vs `idVendaProduto2`, `has_endereco`
- **Inconsistent Data Types**: Mixed INT(10), INT(11), VARCHAR lengths
- **Varying Code Styles**: Different error handling patterns across modules
- **Ad-hoc Solutions**: Quick fixes leading to technical debt

#### **Module Boundaries**
Current modules lack clear boundaries:
- **Compras**: Purchase logic scattered across multiple widgets
- **Estoque**: Inventory management mixed with sales logic
- **Financeiro**: Financial calculations in database views
- **NFe**: Brazilian compliance code tightly coupled with sales

### 5. Performance and Scalability Issues

#### **Database Performance Problems**
1. **Complex View Queries**: Views with 10+ table joins affecting performance
2. **Missing Indexes**: Limited indexing strategy for large datasets
3. **No Query Optimization**: No caching or query optimization patterns

#### **Application Performance Issues**
1. **Synchronous UI**: Blocking operations in main thread
2. **No Background Processing**: All operations synchronous
3. **Memory Usage**: Qt widgets loaded simultaneously without lazy loading

### 6. Maintenance and Development Issues

#### **Testing Challenges**
- **No Unit Tests**: Business logic embedded in UI makes testing impossible
- **No Integration Tests**: Database-dependent code can't be easily tested
- **Manual Testing Only**: Requires full system for any testing

#### **Development Velocity Issues**
- **Long Build Times**: Monolithic structure requires full rebuilds
- **Developer Onboarding**: Complex, undocumented architecture
- **Feature Development**: Adding features requires understanding entire system

## Proposed Modern Architecture

### 1. Clean Architecture Design

#### **Hexagonal Architecture (Ports and Adapters)**

```
┌─────────────────────────────────────────────────────────────┐
│                    Presentation Layer                        │
│  ┌─────────────────┐ ┌─────────────────┐ ┌───────────────┐  │
│  │   Web Frontend  │ │   Mobile App    │ │   Admin Panel │  │
│  │   (React/Vue)   │ │   (Flutter)     │ │   (React)     │  │
│  └─────────────────┘ └─────────────────┘ └───────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                         │
│  ┌─────────────────┐ ┌─────────────────┐ ┌───────────────┐  │
│  │   REST API      │ │   GraphQL API   │ │   WebSockets  │  │
│  │   Controllers   │ │   Resolvers     │ │   Handlers    │  │
│  └─────────────────┘ └─────────────────┘ └───────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                    Domain Layer (Core)                       │
│  ┌─────────────────┐ ┌─────────────────┐ ┌───────────────┐  │
│  │   Use Cases     │ │   Entities      │ │   Services    │  │
│  │   (Business     │ │   (Domain       │ │   (Domain     │  │
│  │   Logic)        │ │   Models)       │ │   Logic)      │  │
│  └─────────────────┘ └─────────────────┘ └───────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                   Infrastructure Layer                       │
│  ┌─────────────────┐ ┌─────────────────┐ ┌───────────────┐  │
│  │   Database      │ │   File Storage  │ │   External    │  │
│  │   Repositories  │ │   Services      │ │   APIs        │  │
│  └─────────────────┘ └─────────────────┘ └───────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### 2. Domain-Driven Design (DDD) Approach

#### **Identified Bounded Contexts**

```typescript
// 1. Sales Context
namespace Sales {
  export interface Quote {
    id: QuoteId;
    customerId: CustomerId;
    items: QuoteItem[];
    total: Money;
    status: QuoteStatus;
    validUntil: Date;
  }

  export interface Sale {
    id: SaleId;
    quoteId?: QuoteId;
    customerId: CustomerId;
    items: SaleItem[];
    total: Money;
    status: SaleStatus;
    deliveryDate: Date;
  }
}

// 2. Inventory Context
namespace Inventory {
  export interface Product {
    id: ProductId;
    sku: SKU;
    description: string;
    supplier: SupplierId;
    price: Money;
    specifications: ProductSpecs;
  }

  export interface Stock {
    id: StockId;
    productId: ProductId;
    location: LocationId;
    quantity: Quantity;
    batch: BatchNumber;
    expiryDate?: Date;
  }
}

// 3. Purchasing Context
namespace Purchasing {
  export interface PurchaseOrder {
    id: PurchaseOrderId;
    supplierId: SupplierId;
    items: PurchaseOrderItem[];
    status: PurchaseOrderStatus;
    expectedDelivery: Date;
  }
}

// 4. Financial Context
namespace Financial {
  export interface AccountsPayable {
    id: PayableId;
    supplierId: SupplierId;
    amount: Money;
    dueDate: Date;
    status: PaymentStatus;
  }

  export interface AccountsReceivable {
    id: ReceivableId;
    customerId: CustomerId;
    amount: Money;
    dueDate: Date;
    status: PaymentStatus;
  }
}

// 5. Logistics Context
namespace Logistics {
  export interface Delivery {
    id: DeliveryId;
    saleId: SaleId;
    address: Address;
    scheduledDate: Date;
    status: DeliveryStatus;
    vehicle?: VehicleId;
  }
}

// 6. Compliance Context (Brazilian NFe)
namespace Compliance {
  export interface ElectronicInvoice {
    id: InvoiceId;
    saleId: SaleId;
    accessKey: string;
    xmlContent: string;
    status: InvoiceStatus;
    issuedAt: DateTime;
  }
}
```

### 3. Redesigned Database Schema

#### **Core Entities with Proper Separation**

```sql
-- =====================================================
-- CORE BUSINESS ENTITIES
-- =====================================================

-- Company/Store Management
CREATE TABLE companies (
    id UUID PRIMARY KEY,
    legal_name VARCHAR(100) NOT NULL,
    trade_name VARCHAR(100),
    tax_id VARCHAR(20) UNIQUE NOT NULL,
    state_registration VARCHAR(20),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

CREATE TABLE company_addresses (
    id UUID PRIMARY KEY,
    company_id UUID NOT NULL REFERENCES companies(id),
    type ENUM('main', 'billing', 'shipping') NOT NULL,
    street VARCHAR(100) NOT NULL,
    number VARCHAR(20),
    complement VARCHAR(50),
    district VARCHAR(50),
    city VARCHAR(50) NOT NULL,
    state CHAR(2) NOT NULL,
    postal_code VARCHAR(10) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Customer Management
CREATE TABLE customers (
    id UUID PRIMARY KEY,
    company_id UUID NOT NULL REFERENCES companies(id),
    type ENUM('individual', 'business') NOT NULL,
    name VARCHAR(100) NOT NULL,
    email VARCHAR(100),
    phone VARCHAR(20),
    tax_id VARCHAR(20) UNIQUE,
    credit_limit DECIMAL(15,4) DEFAULT 0,
    status ENUM('active', 'inactive', 'blocked') DEFAULT 'active',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_customers_tax_id (tax_id),
    INDEX idx_customers_company (company_id),
    FULLTEXT idx_customers_search (name, email)
);

-- Product Catalog
CREATE TABLE product_categories (
    id UUID PRIMARY KEY,
    name VARCHAR(50) NOT NULL,
    description TEXT,
    parent_id UUID REFERENCES product_categories(id),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE suppliers (
    id UUID PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    tax_id VARCHAR(20) UNIQUE NOT NULL,
    contact_email VARCHAR(100),
    contact_phone VARCHAR(20),
    commission_rate DECIMAL(5,4) DEFAULT 0,
    status ENUM('active', 'inactive') DEFAULT 'active',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

CREATE TABLE products (
    id UUID PRIMARY KEY,
    supplier_id UUID NOT NULL REFERENCES suppliers(id),
    category_id UUID REFERENCES product_categories(id),
    sku VARCHAR(50) UNIQUE NOT NULL,
    name VARCHAR(100) NOT NULL,
    description TEXT,
    unit_of_measure VARCHAR(10) NOT NULL,
    ncm_code VARCHAR(10), -- Brazilian tax classification
    cost_price DECIMAL(15,4) NOT NULL,
    sale_price DECIMAL(15,4) NOT NULL,
    weight DECIMAL(10,3),
    dimensions JSON, -- {"length": 10, "width": 5, "height": 2}
    status ENUM('active', 'inactive', 'discontinued') DEFAULT 'active',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_products_sku (sku),
    INDEX idx_products_supplier (supplier_id),
    FULLTEXT idx_products_search (name, description)
);

-- =====================================================
-- INVENTORY MANAGEMENT
-- =====================================================

CREATE TABLE warehouses (
    id UUID PRIMARY KEY,
    company_id UUID NOT NULL REFERENCES companies(id),
    name VARCHAR(50) NOT NULL,
    address JSON NOT NULL, -- Store address information
    capacity JSON, -- Store capacity information
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE inventory_batches (
    id UUID PRIMARY KEY,
    product_id UUID NOT NULL REFERENCES products(id),
    warehouse_id UUID NOT NULL REFERENCES warehouses(id),
    batch_number VARCHAR(50) NOT NULL,
    quantity DECIMAL(15,4) NOT NULL,
    reserved_quantity DECIMAL(15,4) DEFAULT 0,
    unit_cost DECIMAL(15,4) NOT NULL,
    expiry_date DATE,
    received_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    status ENUM('available', 'reserved', 'expired', 'damaged') DEFAULT 'available',
    UNIQUE KEY unique_batch (product_id, warehouse_id, batch_number),
    INDEX idx_inventory_product (product_id),
    INDEX idx_inventory_warehouse (warehouse_id)
);

-- =====================================================
-- SALES MANAGEMENT
-- =====================================================

CREATE TABLE quotes (
    id UUID PRIMARY KEY,
    quote_number VARCHAR(20) UNIQUE NOT NULL,
    company_id UUID NOT NULL REFERENCES companies(id),
    customer_id UUID NOT NULL REFERENCES customers(id),
    salesperson_id UUID NOT NULL REFERENCES users(id),
    subtotal DECIMAL(15,4) NOT NULL,
    discount DECIMAL(15,4) DEFAULT 0,
    shipping_cost DECIMAL(15,4) DEFAULT 0,
    total DECIMAL(15,4) NOT NULL,
    valid_until DATE NOT NULL,
    status ENUM('draft', 'sent', 'approved', 'rejected', 'expired') DEFAULT 'draft',
    notes TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_quotes_customer (customer_id),
    INDEX idx_quotes_status (status),
    INDEX idx_quotes_date (created_at)
);

CREATE TABLE quote_items (
    id UUID PRIMARY KEY,
    quote_id UUID NOT NULL REFERENCES quotes(id) ON DELETE CASCADE,
    product_id UUID NOT NULL REFERENCES products(id),
    quantity DECIMAL(15,4) NOT NULL,
    unit_price DECIMAL(15,4) NOT NULL,
    discount DECIMAL(15,4) DEFAULT 0,
    total DECIMAL(15,4) NOT NULL,
    notes TEXT,
    line_number INT NOT NULL,
    INDEX idx_quote_items_quote (quote_id),
    UNIQUE KEY unique_line (quote_id, line_number)
);

CREATE TABLE sales (
    id UUID PRIMARY KEY,
    sale_number VARCHAR(20) UNIQUE NOT NULL,
    quote_id UUID REFERENCES quotes(id),
    company_id UUID NOT NULL REFERENCES companies(id),
    customer_id UUID NOT NULL REFERENCES customers(id),
    salesperson_id UUID NOT NULL REFERENCES users(id),
    subtotal DECIMAL(15,4) NOT NULL,
    discount DECIMAL(15,4) DEFAULT 0,
    shipping_cost DECIMAL(15,4) DEFAULT 0,
    total DECIMAL(15,4) NOT NULL,
    status ENUM('pending', 'confirmed', 'invoiced', 'delivered', 'cancelled') DEFAULT 'pending',
    expected_delivery DATE,
    notes TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_sales_customer (customer_id),
    INDEX idx_sales_status (status),
    INDEX idx_sales_date (created_at)
);

CREATE TABLE sale_items (
    id UUID PRIMARY KEY,
    sale_id UUID NOT NULL REFERENCES sales(id) ON DELETE CASCADE,
    product_id UUID NOT NULL REFERENCES products(id),
    quantity DECIMAL(15,4) NOT NULL,
    unit_price DECIMAL(15,4) NOT NULL,
    discount DECIMAL(15,4) DEFAULT 0,
    total DECIMAL(15,4) NOT NULL,
    line_number INT NOT NULL,
    INDEX idx_sale_items_sale (sale_id),
    UNIQUE KEY unique_line (sale_id, line_number)
);

-- =====================================================
-- INVENTORY MOVEMENTS
-- =====================================================

CREATE TABLE inventory_movements (
    id UUID PRIMARY KEY,
    batch_id UUID NOT NULL REFERENCES inventory_batches(id),
    movement_type ENUM('inbound', 'outbound', 'adjustment', 'transfer') NOT NULL,
    reference_type ENUM('purchase', 'sale', 'adjustment', 'transfer') NOT NULL,
    reference_id UUID NOT NULL, -- References purchase_orders, sales, etc.
    quantity DECIMAL(15,4) NOT NULL, -- Positive for inbound, negative for outbound
    unit_cost DECIMAL(15,4),
    notes TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    created_by UUID NOT NULL REFERENCES users(id),
    INDEX idx_movements_batch (batch_id),
    INDEX idx_movements_reference (reference_type, reference_id),
    INDEX idx_movements_date (created_at)
);

-- =====================================================
-- FINANCIAL MANAGEMENT
-- =====================================================

CREATE TABLE payment_methods (
    id UUID PRIMARY KEY,
    name VARCHAR(50) NOT NULL,
    type ENUM('cash', 'card', 'transfer', 'check', 'pix') NOT NULL,
    active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE accounts_receivable (
    id UUID PRIMARY KEY,
    sale_id UUID NOT NULL REFERENCES sales(id),
    customer_id UUID NOT NULL REFERENCES customers(id),
    amount DECIMAL(15,4) NOT NULL,
    due_date DATE NOT NULL,
    status ENUM('pending', 'paid', 'overdue', 'cancelled') DEFAULT 'pending',
    payment_method_id UUID REFERENCES payment_methods(id),
    paid_amount DECIMAL(15,4) DEFAULT 0,
    paid_at TIMESTAMP NULL,
    notes TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_receivable_customer (customer_id),
    INDEX idx_receivable_due_date (due_date),
    INDEX idx_receivable_status (status)
);

CREATE TABLE accounts_payable (
    id UUID PRIMARY KEY,
    purchase_order_id UUID REFERENCES purchase_orders(id),
    supplier_id UUID NOT NULL REFERENCES suppliers(id),
    amount DECIMAL(15,4) NOT NULL,
    due_date DATE NOT NULL,
    status ENUM('pending', 'paid', 'overdue', 'cancelled') DEFAULT 'pending',
    payment_method_id UUID REFERENCES payment_methods(id),
    paid_amount DECIMAL(15,4) DEFAULT 0,
    paid_at TIMESTAMP NULL,
    notes TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_payable_supplier (supplier_id),
    INDEX idx_payable_due_date (due_date),
    INDEX idx_payable_status (status)
);

-- =====================================================
-- BRAZILIAN COMPLIANCE (NFe)
-- =====================================================

CREATE TABLE electronic_invoices (
    id UUID PRIMARY KEY,
    sale_id UUID NOT NULL REFERENCES sales(id),
    company_id UUID NOT NULL REFERENCES companies(id),
    invoice_number VARCHAR(20) NOT NULL,
    series VARCHAR(10) NOT NULL,
    access_key VARCHAR(44) UNIQUE NOT NULL,
    xml_content LONGTEXT NOT NULL,
    status ENUM('draft', 'authorized', 'cancelled', 'rejected') DEFAULT 'draft',
    issued_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    authorized_at TIMESTAMP NULL,
    protocol_number VARCHAR(50),
    rejection_reason TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_nfe_sale (sale_id),
    INDEX idx_nfe_access_key (access_key),
    INDEX idx_nfe_status (status)
);

-- =====================================================
-- USER MANAGEMENT
-- =====================================================

CREATE TABLE users (
    id UUID PRIMARY KEY,
    company_id UUID NOT NULL REFERENCES companies(id),
    username VARCHAR(50) UNIQUE NOT NULL,
    email VARCHAR(100) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    full_name VARCHAR(100) NOT NULL,
    role ENUM('admin', 'manager', 'salesperson', 'operator') NOT NULL,
    active BOOLEAN DEFAULT TRUE,
    last_login TIMESTAMP NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_users_company (company_id),
    INDEX idx_users_role (role)
);

-- =====================================================
-- AUDIT TRAIL
-- =====================================================

CREATE TABLE audit_logs (
    id UUID PRIMARY KEY,
    table_name VARCHAR(50) NOT NULL,
    record_id UUID NOT NULL,
    action ENUM('insert', 'update', 'delete') NOT NULL,
    old_values JSON,
    new_values JSON,
    changed_by UUID NOT NULL REFERENCES users(id),
    changed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_audit_table_record (table_name, record_id),
    INDEX idx_audit_user (changed_by),
    INDEX idx_audit_date (changed_at)
);
```

### 4. Modern Application Architecture

#### **Microservices Architecture**

```typescript
// =====================================================
// SERVICE LAYER ARCHITECTURE
// =====================================================

// Base Service Interface
interface BaseService<T, TId> {
  findById(id: TId): Promise<T | null>;
  findAll(filters?: any): Promise<T[]>;
  create(entity: Omit<T, 'id'>): Promise<T>;
  update(id: TId, updates: Partial<T>): Promise<T>;
  delete(id: TId): Promise<void>;
}

// Sales Service
interface SalesService extends BaseService<Sale, SaleId> {
  createFromQuote(quoteId: QuoteId): Promise<Sale>;
  addItem(saleId: SaleId, item: SaleItemRequest): Promise<SaleItem>;
  updateStatus(saleId: SaleId, status: SaleStatus): Promise<Sale>;
  calculateTotals(saleId: SaleId): Promise<SaleTotals>;
  findBySalesperson(salespersonId: UserId): Promise<Sale[]>;
  findByCustomer(customerId: CustomerId): Promise<Sale[]>;
}

// Inventory Service
interface InventoryService extends BaseService<InventoryBatch, BatchId> {
  reserveStock(productId: ProductId, quantity: number): Promise<ReservationResult>;
  releaseReservation(reservationId: ReservationId): Promise<void>;
  moveStock(moveRequest: StockMovementRequest): Promise<StockMovement>;
  checkAvailability(productId: ProductId, quantity: number): Promise<AvailabilityResult>;
  getStockLevels(warehouseId?: WarehouseId): Promise<StockLevel[]>;
  adjustStock(adjustmentRequest: StockAdjustmentRequest): Promise<void>;
}

// Financial Service
interface FinancialService {
  createReceivable(saleId: SaleId, amount: Money, dueDate: Date): Promise<AccountsReceivable>;
  createPayable(purchaseOrderId: PurchaseOrderId, amount: Money, dueDate: Date): Promise<AccountsPayable>;
  processPayment(paymentRequest: PaymentRequest): Promise<PaymentResult>;
  generateCashFlow(dateRange: DateRange): Promise<CashFlowReport>;
  getOverdueAccounts(): Promise<OverdueAccount[]>;
}

// Brazilian Compliance Service
interface ComplianceService {
  generateElectronicInvoice(saleId: SaleId): Promise<ElectronicInvoice>;
  submitToSefaz(invoiceId: InvoiceId): Promise<SefazResponse>;
  checkInvoiceStatus(accessKey: string): Promise<InvoiceStatus>;
  generateDanfe(invoiceId: InvoiceId): Promise<Buffer>; // PDF bytes
  cancelInvoice(invoiceId: InvoiceId, reason: string): Promise<void>;
}
```

#### **Domain Events and CQRS**

```typescript
// =====================================================
// DOMAIN EVENTS
// =====================================================

// Base Domain Event
abstract class DomainEvent {
  abstract readonly eventType: string;
  readonly occurredAt: Date = new Date();
  readonly eventId: string = crypto.randomUUID();
}

// Sales Events
class SaleCreatedEvent extends DomainEvent {
  readonly eventType = 'SaleCreated';
  constructor(public readonly sale: Sale) { super(); }
}

class SaleItemAddedEvent extends DomainEvent {
  readonly eventType = 'SaleItemAdded';
  constructor(
    public readonly saleId: SaleId,
    public readonly item: SaleItem
  ) { super(); }
}

class SaleConfirmedEvent extends DomainEvent {
  readonly eventType = 'SaleConfirmed';
  constructor(public readonly sale: Sale) { super(); }
}

// Inventory Events
class StockReservedEvent extends DomainEvent {
  readonly eventType = 'StockReserved';
  constructor(
    public readonly productId: ProductId,
    public readonly quantity: number,
    public readonly reservationId: ReservationId
  ) { super(); }
}

class StockMovedEvent extends DomainEvent {
  readonly eventType = 'StockMoved';
  constructor(public readonly movement: StockMovement) { super(); }
}

// Event Handlers
@EventHandler(SaleConfirmedEvent)
class SaleConfirmedHandler {
  constructor(
    private inventoryService: InventoryService,
    private financialService: FinancialService,
    private complianceService: ComplianceService
  ) {}

  async handle(event: SaleConfirmedEvent): Promise<void> {
    const sale = event.sale;

    // Reserve inventory
    for (const item of sale.items) {
      await this.inventoryService.reserveStock(item.productId, item.quantity);
    }

    // Create accounts receivable
    await this.financialService.createReceivable(
      sale.id,
      sale.total,
      sale.expectedDelivery
    );

    // Generate NFe if required
    if (sale.requiresElectronicInvoice) {
      await this.complianceService.generateElectronicInvoice(sale.id);
    }
  }
}

// =====================================================
// CQRS - COMMAND QUERY RESPONSIBILITY SEGREGATION
// =====================================================

// Commands (Write Side)
interface CreateSaleCommand {
  customerId: CustomerId;
  items: CreateSaleItemCommand[];
  expectedDelivery: Date;
  notes?: string;
}

interface AddSaleItemCommand {
  saleId: SaleId;
  productId: ProductId;
  quantity: number;
  unitPrice: Money;
  discount?: number;
}

interface ConfirmSaleCommand {
  saleId: SaleId;
  confirmedBy: UserId;
}

// Queries (Read Side)
interface SaleDetailsQuery {
  saleId: SaleId;
  includeItems: boolean;
  includeCustomer: boolean;
  includePayments: boolean;
}

interface SalesReportQuery {
  dateRange: DateRange;
  salespersonId?: UserId;
  customerId?: CustomerId;
  status?: SaleStatus[];
  groupBy?: 'day' | 'week' | 'month' | 'salesperson' | 'customer';
}

interface InventoryReportQuery {
  warehouseId?: WarehouseId;
  productId?: ProductId;
  includeReservations: boolean;
  includeMovements: boolean;
  stockLevel?: 'all' | 'low' | 'zero' | 'negative';
}

// Command Handlers
@CommandHandler(CreateSaleCommand)
class CreateSaleHandler {
  constructor(
    private salesService: SalesService,
    private eventBus: EventBus
  ) {}

  async handle(command: CreateSaleCommand): Promise<Sale> {
    const sale = await this.salesService.create({
      customerId: command.customerId,
      items: command.items,
      expectedDelivery: command.expectedDelivery,
      notes: command.notes,
      status: SaleStatus.PENDING
    });

    await this.eventBus.publish(new SaleCreatedEvent(sale));
    return sale;
  }
}

// Query Handlers
@QueryHandler(SalesReportQuery)
class SalesReportHandler {
  constructor(private salesReadService: SalesReadService) {}

  async handle(query: SalesReportQuery): Promise<SalesReport> {
    return await this.salesReadService.generateReport(query);
  }
}
```

### 5. API Design

#### **REST API Design**

```typescript
// =====================================================
// REST API CONTROLLERS
// =====================================================

@Controller('/api/v1/sales')
export class SalesController {
  constructor(
    private commandBus: CommandBus,
    private queryBus: QueryBus
  ) {}

  @Post('/')
  @ApiOperation({ summary: 'Create a new sale' })
  @ApiResponse({ status: 201, description: 'Sale created successfully' })
  async createSale(@Body() request: CreateSaleRequest): Promise<SaleResponse> {
    const command = new CreateSaleCommand(
      request.customerId,
      request.items,
      request.expectedDelivery,
      request.notes
    );

    const sale = await this.commandBus.execute(command);
    return SaleResponse.fromDomain(sale);
  }

  @Get('/:id')
  @ApiOperation({ summary: 'Get sale details' })
  async getSale(
    @Param('id') id: string,
    @Query() options: SaleDetailsOptions
  ): Promise<SaleDetailsResponse> {
    const query = new SaleDetailsQuery(id, options);
    const result = await this.queryBus.execute(query);
    return SaleDetailsResponse.fromDomain(result);
  }

  @Post('/:id/items')
  @ApiOperation({ summary: 'Add item to sale' })
  async addItem(
    @Param('id') saleId: string,
    @Body() request: AddSaleItemRequest
  ): Promise<SaleItemResponse> {
    const command = new AddSaleItemCommand(
      saleId,
      request.productId,
      request.quantity,
      request.unitPrice,
      request.discount
    );

    const item = await this.commandBus.execute(command);
    return SaleItemResponse.fromDomain(item);
  }

  @Patch('/:id/status')
  @ApiOperation({ summary: 'Update sale status' })
  async updateStatus(
    @Param('id') saleId: string,
    @Body() request: UpdateSaleStatusRequest
  ): Promise<SaleResponse> {
    const command = new UpdateSaleStatusCommand(saleId, request.status);
    const sale = await this.commandBus.execute(command);
    return SaleResponse.fromDomain(sale);
  }

  @Get('/reports/summary')
  @ApiOperation({ summary: 'Generate sales report' })
  async getSalesReport(
    @Query() filters: SalesReportFilters
  ): Promise<SalesReportResponse> {
    const query = new SalesReportQuery(filters);
    const report = await this.queryBus.execute(query);
    return SalesReportResponse.fromDomain(report);
  }
}

@Controller('/api/v1/inventory')
export class InventoryController {
  constructor(
    private commandBus: CommandBus,
    private queryBus: QueryBus
  ) {}

  @Get('/products/:id/availability')
  @ApiOperation({ summary: 'Check product availability' })
  async checkAvailability(
    @Param('id') productId: string,
    @Query('quantity') quantity: number
  ): Promise<AvailabilityResponse> {
    const query = new CheckAvailabilityQuery(productId, quantity);
    const result = await this.queryBus.execute(query);
    return AvailabilityResponse.fromDomain(result);
  }

  @Post('/movements')
  @ApiOperation({ summary: 'Record inventory movement' })
  async recordMovement(
    @Body() request: InventoryMovementRequest
  ): Promise<MovementResponse> {
    const command = new RecordMovementCommand(request);
    const movement = await this.commandBus.execute(command);
    return MovementResponse.fromDomain(movement);
  }

  @Get('/stock-levels')
  @ApiOperation({ summary: 'Get current stock levels' })
  async getStockLevels(
    @Query() filters: StockLevelFilters
  ): Promise<StockLevelResponse[]> {
    const query = new StockLevelsQuery(filters);
    const levels = await this.queryBus.execute(query);
    return levels.map(level => StockLevelResponse.fromDomain(level));
  }
}

@Controller('/api/v1/compliance/nfe')
export class NFeController {
  constructor(
    private commandBus: CommandBus,
    private queryBus: QueryBus
  ) {}

  @Post('/generate')
  @ApiOperation({ summary: 'Generate electronic invoice (NFe)' })
  async generateInvoice(
    @Body() request: GenerateNFeRequest
  ): Promise<NFeResponse> {
    const command = new GenerateNFeCommand(request.saleId);
    const invoice = await this.commandBus.execute(command);
    return NFeResponse.fromDomain(invoice);
  }

  @Post('/:id/submit')
  @ApiOperation({ summary: 'Submit NFe to SEFAZ' })
  async submitToSefaz(
    @Param('id') invoiceId: string
  ): Promise<SefazSubmissionResponse> {
    const command = new SubmitNFeCommand(invoiceId);
    const result = await this.commandBus.execute(command);
    return SefazSubmissionResponse.fromDomain(result);
  }

  @Get('/:id/danfe')
  @ApiOperation({ summary: 'Generate DANFE PDF' })
  async generateDanfe(
    @Param('id') invoiceId: string,
    @Res() response: Response
  ): Promise<void> {
    const query = new GenerateDANFEQuery(invoiceId);
    const pdfBuffer = await this.queryBus.execute(query);

    response.setHeader('Content-Type', 'application/pdf');
    response.setHeader('Content-Disposition', `attachment; filename="danfe-${invoiceId}.pdf"`);
    response.send(pdfBuffer);
  }
}
```

### 6. Frontend Architecture

#### **Modern React Application Structure**

```typescript
// =====================================================
// FRONTEND ARCHITECTURE
// =====================================================

// State Management with Redux Toolkit
import { createSlice, createAsyncThunk } from '@reduxjs/toolkit';

// Sales Slice
export const fetchSales = createAsyncThunk(
  'sales/fetchSales',
  async (filters: SalesFilters) => {
    const response = await salesApi.getSales(filters);
    return response.data;
  }
);

const salesSlice = createSlice({
  name: 'sales',
  initialState: {
    items: [],
    loading: false,
    error: null,
    selectedSale: null,
    filters: defaultFilters
  },
  reducers: {
    setFilters: (state, action) => {
      state.filters = action.payload;
    },
    selectSale: (state, action) => {
      state.selectedSale = action.payload;
    }
  },
  extraReducers: (builder) => {
    builder
      .addCase(fetchSales.pending, (state) => {
        state.loading = true;
      })
      .addCase(fetchSales.fulfilled, (state, action) => {
        state.loading = false;
        state.items = action.payload;
      })
      .addCase(fetchSales.rejected, (state, action) => {
        state.loading = false;
        state.error = action.error.message;
      });
  }
});

// React Components with TypeScript
interface SalesListProps {
  filters?: SalesFilters;
  onSaleSelect?: (sale: Sale) => void;
}

export const SalesList: React.FC<SalesListProps> = ({ filters, onSaleSelect }) => {
  const dispatch = useAppDispatch();
  const { items: sales, loading, error } = useAppSelector(state => state.sales);

  useEffect(() => {
    dispatch(fetchSales(filters || {}));
  }, [dispatch, filters]);

  const handleSaleClick = useCallback((sale: Sale) => {
    dispatch(selectSale(sale));
    onSaleSelect?.(sale);
  }, [dispatch, onSaleSelect]);

  if (loading) return <LoadingSpinner />;
  if (error) return <ErrorMessage message={error} />;

  return (
    <div className="sales-list">
      <div className="sales-header">
        <h2>Sales</h2>
        <SalesFilters
          filters={filters}
          onFiltersChange={(newFilters) => dispatch(setFilters(newFilters))}
        />
      </div>

      <DataTable
        data={sales}
        columns={salesTableColumns}
        onRowClick={handleSaleClick}
        pagination
        sortable
      />
    </div>
  );
};

// Custom Hooks for Business Logic
export const useSaleManagement = () => {
  const dispatch = useAppDispatch();
  const [loading, setLoading] = useState(false);

  const createSale = useCallback(async (saleData: CreateSaleData) => {
    setLoading(true);
    try {
      const sale = await salesApi.createSale(saleData);
      dispatch(addSale(sale));
      return sale;
    } catch (error) {
      throw error;
    } finally {
      setLoading(false);
    }
  }, [dispatch]);

  const addSaleItem = useCallback(async (saleId: string, itemData: SaleItemData) => {
    setLoading(true);
    try {
      const item = await salesApi.addSaleItem(saleId, itemData);
      dispatch(updateSaleItems({ saleId, item }));
      return item;
    } catch (error) {
      throw error;
    } finally {
      setLoading(false);
    }
  }, [dispatch]);

  const confirmSale = useCallback(async (saleId: string) => {
    setLoading(true);
    try {
      const sale = await salesApi.confirmSale(saleId);
      dispatch(updateSale(sale));
      // Trigger notifications, updates, etc.
      return sale;
    } catch (error) {
      throw error;
    } finally {
      setLoading(false);
    }
  }, [dispatch]);

  return {
    createSale,
    addSaleItem,
    confirmSale,
    loading
  };
};

// Form Management with React Hook Form
interface SaleFormData {
  customerId: string;
  items: SaleItemFormData[];
  expectedDelivery: string;
  notes: string;
}

export const SaleForm: React.FC = () => {
  const { control, handleSubmit, watch, formState: { errors } } = useForm<SaleFormData>();
  const { createSale, loading } = useSaleManagement();

  const onSubmit = async (data: SaleFormData) => {
    try {
      await createSale({
        customerId: data.customerId,
        items: data.items,
        expectedDelivery: new Date(data.expectedDelivery),
        notes: data.notes
      });
      // Show success message, redirect, etc.
    } catch (error) {
      // Handle error
    }
  };

  return (
    <form onSubmit={handleSubmit(onSubmit)} className="sale-form">
      <Controller
        name="customerId"
        control={control}
        rules={{ required: 'Customer is required' }}
        render={({ field }) => (
          <CustomerSelect
            {...field}
            error={errors.customerId?.message}
          />
        )}
      />

      <SaleItemsFieldArray control={control} />

      <Controller
        name="expectedDelivery"
        control={control}
        rules={{ required: 'Delivery date is required' }}
        render={({ field }) => (
          <DatePicker
            {...field}
            label="Expected Delivery"
            error={errors.expectedDelivery?.message}
          />
        )}
      />

      <Controller
        name="notes"
        control={control}
        render={({ field }) => (
          <TextArea
            {...field}
            label="Notes"
            rows={3}
          />
        )}
      />

      <div className="form-actions">
        <Button type="button" variant="outline">Cancel</Button>
        <Button type="submit" loading={loading}>Create Sale</Button>
      </div>
    </form>
  );
};
```

### 7. Testing Strategy

#### **Comprehensive Testing Approach**

```typescript
// =====================================================
// UNIT TESTS
// =====================================================

// Domain Entity Tests
describe('Sale Entity', () => {
  let sale: Sale;

  beforeEach(() => {
    sale = new Sale({
      id: SaleId.generate(),
      customerId: CustomerId.generate(),
      items: [],
      status: SaleStatus.PENDING
    });
  });

  describe('addItem', () => {
    it('should add item to sale', () => {
      const item = new SaleItem({
        productId: ProductId.generate(),
        quantity: 10,
        unitPrice: Money.from(100),
        discount: Money.from(0)
      });

      sale.addItem(item);

      expect(sale.items).toHaveLength(1);
      expect(sale.items[0]).toBe(item);
    });

    it('should recalculate totals when item added', () => {
      const item = new SaleItem({
        productId: ProductId.generate(),
        quantity: 10,
        unitPrice: Money.from(100),
        discount: Money.from(10)
      });

      sale.addItem(item);

      expect(sale.subtotal.amount).toBe(990); // (10 * 100) - 10
    });

    it('should throw error when adding item to confirmed sale', () => {
      sale.confirm();

      const item = new SaleItem({
        productId: ProductId.generate(),
        quantity: 10,
        unitPrice: Money.from(100),
        discount: Money.from(0)
      });

      expect(() => sale.addItem(item)).toThrow('Cannot add items to confirmed sale');
    });
  });
});

// Service Tests with Mocking
describe('SalesService', () => {
  let salesService: SalesService;
  let mockRepository: jest.Mocked<SalesRepository>;
  let mockEventBus: jest.Mocked<EventBus>;

  beforeEach(() => {
    mockRepository = createMockRepository();
    mockEventBus = createMockEventBus();
    salesService = new SalesService(mockRepository, mockEventBus);
  });

  describe('createSale', () => {
    it('should create sale and publish event', async () => {
      const saleData = {
        customerId: CustomerId.generate(),
        items: [createSaleItemData()],
        expectedDelivery: new Date(),
        notes: 'Test sale'
      };

      mockRepository.save.mockResolvedValue(createSale(saleData));

      const result = await salesService.create(saleData);

      expect(mockRepository.save).toHaveBeenCalledWith(expect.any(Sale));
      expect(mockEventBus.publish).toHaveBeenCalledWith(
        expect.any(SaleCreatedEvent)
      );
      expect(result.customerId).toBe(saleData.customerId);
    });
  });

  describe('confirmSale', () => {
    it('should confirm sale and publish event', async () => {
      const sale = createSale();
      mockRepository.findById.mockResolvedValue(sale);
      mockRepository.save.mockResolvedValue(sale);

      await salesService.confirm(sale.id);

      expect(sale.status).toBe(SaleStatus.CONFIRMED);
      expect(mockEventBus.publish).toHaveBeenCalledWith(
        expect.any(SaleConfirmedEvent)
      );
    });

    it('should throw error when sale not found', async () => {
      mockRepository.findById.mockResolvedValue(null);

      await expect(salesService.confirm(SaleId.generate()))
        .rejects.toThrow('Sale not found');
    });
  });
});

// =====================================================
// INTEGRATION TESTS
// =====================================================

describe('Sales API Integration', () => {
  let app: INestApplication;
  let database: TestDatabase;

  beforeAll(async () => {
    const moduleRef = await Test.createTestingModule({
      imports: [AppModule],
    }).compile();

    app = moduleRef.createNestApplication();
    await app.init();

    database = new TestDatabase();
    await database.setup();
  });

  afterAll(async () => {
    await database.cleanup();
    await app.close();
  });

  afterEach(async () => {
    await database.reset();
  });

  describe('POST /api/v1/sales', () => {
    it('should create sale successfully', async () => {
      const customer = await database.createCustomer();
      const product = await database.createProduct();

      const saleData = {
        customerId: customer.id,
        items: [{
          productId: product.id,
          quantity: 10,
          unitPrice: 100,
          discount: 0
        }],
        expectedDelivery: '2025-12-31',
        notes: 'Test sale'
      };

      const response = await request(app.getHttpServer())
        .post('/api/v1/sales')
        .send(saleData)
        .expect(201);

      expect(response.body).toMatchObject({
        id: expect.any(String),
        customerId: customer.id,
        total: 1000,
        status: 'PENDING'
      });

      // Verify database state
      const sale = await database.findSale(response.body.id);
      expect(sale).toBeDefined();
      expect(sale.items).toHaveLength(1);
    });

    it('should return 400 for invalid data', async () => {
      const invalidData = {
        customerId: 'invalid-uuid',
        items: [],
        expectedDelivery: 'invalid-date'
      };

      const response = await request(app.getHttpServer())
        .post('/api/v1/sales')
        .send(invalidData)
        .expect(400);

      expect(response.body.message).toContain('validation failed');
    });
  });

  describe('POST /api/v1/sales/:id/confirm', () => {
    it('should confirm sale and trigger side effects', async () => {
      const sale = await database.createSale();
      const initialStockLevel = await database.getStockLevel(sale.items[0].productId);

      await request(app.getHttpServer())
        .post(`/api/v1/sales/${sale.id}/confirm`)
        .expect(200);

      // Verify sale status updated
      const updatedSale = await database.findSale(sale.id);
      expect(updatedSale.status).toBe('CONFIRMED');

      // Verify stock reserved
      const newStockLevel = await database.getStockLevel(sale.items[0].productId);
      expect(newStockLevel.reserved).toBe(
        initialStockLevel.reserved + sale.items[0].quantity
      );

      // Verify accounts receivable created
      const receivables = await database.findReceivablesBySale(sale.id);
      expect(receivables).toHaveLength(1);
      expect(receivables[0].amount).toBe(sale.total);
    });
  });
});

// =====================================================
// END-TO-END TESTS
// =====================================================

describe('Sales Workflow E2E', () => {
  let page: Page;

  beforeAll(async () => {
    page = await browser.newPage();
    await page.goto('http://localhost:3000');
    await loginAsUser(page, 'salesperson@test.com');
  });

  afterAll(async () => {
    await page.close();
  });

  it('should complete full sales workflow', async () => {
    // Create quote
    await page.click('[data-testid="new-quote-button"]');
    await page.fill('[data-testid="customer-select"]', 'Test Customer');
    await page.click('[data-testid="add-item-button"]');
    await page.fill('[data-testid="product-select"]', 'Test Product');
    await page.fill('[data-testid="quantity-input"]', '10');
    await page.click('[data-testid="save-quote-button"]');

    // Verify quote created
    await expect(page.locator('[data-testid="quote-number"]')).toBeVisible();
    const quoteNumber = await page.textContent('[data-testid="quote-number"]');

    // Convert quote to sale
    await page.click('[data-testid="convert-to-sale-button"]');
    await page.fill('[data-testid="delivery-date"]', '2025-12-31');
    await page.click('[data-testid="confirm-sale-button"]');

    // Verify sale created and confirmed
    await expect(page.locator('[data-testid="sale-status"]')).toHaveText('CONFIRMED');

    // Verify inventory reserved
    await page.goto('/inventory');
    await page.fill('[data-testid="product-search"]', 'Test Product');
    await expect(page.locator('[data-testid="reserved-quantity"]')).toHaveText('10');

    // Verify accounts receivable created
    await page.goto('/financial/receivables');
    const receivableRow = page.locator(`[data-testid="receivable-sale-${await page.textContent('[data-testid="sale-number"]')}"]`);
    await expect(receivableRow).toBeVisible();
  });
});
```

### 8. Migration Strategy

#### **Phased Migration Approach**

```typescript
// =====================================================
// MIGRATION PHASES
// =====================================================

// Phase 1: Foundation (Months 1-3)
const Phase1_Foundation = {
  goals: [
    'Set up new technology stack',
    'Create core domain models',
    'Implement basic CRUD operations',
    'Set up CI/CD pipeline'
  ],
  deliverables: [
    'New database schema design',
    'Basic user management system',
    'Product catalog management',
    'Customer management',
    'API foundation with authentication'
  ],
  riskMitigation: [
    'Parallel development with existing system',
    'Data synchronization strategy',
    'Rollback plan for each component'
  ]
};

// Phase 2: Core Business Logic (Months 4-6)
const Phase2_CoreBusiness = {
  goals: [
    'Implement sales workflow',
    'Implement inventory management',
    'Implement basic financial features',
    'Migrate business rules from database views'
  ],
  deliverables: [
    'Quote and sales management',
    'Inventory tracking and movements',
    'Accounts receivable/payable',
    'Basic reporting system',
    'Web frontend for core operations'
  ],
  riskMitigation: [
    'Feature flags for gradual rollout',
    'A/B testing between old and new systems',
    'Comprehensive integration testing'
  ]
};

// Phase 3: Advanced Features (Months 7-9)
const Phase3_AdvancedFeatures = {
  goals: [
    'Implement Brazilian NFe compliance',
    'Implement logistics management',
    'Implement advanced reporting',
    'Implement commission calculations'
  ],
  deliverables: [
    'Complete NFe integration with SEFAZ',
    'Delivery scheduling and tracking',
    'Advanced financial reports',
    'Commission calculation system',
    'Mobile application'
  ],
  riskMitigation: [
    'NFe testing in sandbox environment',
    'Gradual migration of complex business rules',
    'Performance testing under load'
  ]
};

// Phase 4: Optimization and Migration (Months 10-12)
const Phase4_Optimization = {
  goals: [
    'Optimize performance',
    'Complete data migration',
    'Decommission old system',
    'Train users on new system'
  ],
  deliverables: [
    'Performance optimized system',
    'Complete historical data migration',
    'User training materials',
    'Documentation and support system',
    'Old system decommissioning'
  ],
  riskMitigation: [
    'Parallel running period',
    'Gradual user migration',
    'Emergency rollback procedures'
  ]
};

// Data Migration Strategy
interface MigrationPlan {
  tables: TableMigration[];
  views: ViewMigration[];
  dataTransformation: DataTransformation[];
  validation: ValidationRule[];
}

interface TableMigration {
  oldTable: string;
  newTable: string;
  mapping: FieldMapping[];
  transformation?: (row: any) => any;
  validation?: (row: any) => boolean;
}

const salesMigration: TableMigration = {
  oldTable: 'venda',
  newTable: 'sales',
  mapping: [
    { from: 'idVenda', to: 'id', transform: (id) => uuidFromLegacyId(id) },
    { from: 'idCliente', to: 'customer_id', transform: (id) => uuidFromLegacyId(id) },
    { from: 'data', to: 'created_at' },
    { from: 'total', to: 'total' },
    { from: 'status', to: 'status', transform: mapLegacyStatus }
  ],
  transformation: (row) => ({
    ...row,
    sale_number: generateSaleNumber(row.id),
    company_id: getCompanyIdFromLoja(row.idLoja)
  }),
  validation: (row) => row.total > 0 && row.customer_id !== null
};

// View Migration to Application Logic
const viewMigrations: ViewMigration[] = [
  {
    viewName: 'EDU_view_Vendas',
    targetService: 'SalesReportService',
    targetMethod: 'generateSalesReport',
    businessLogic: extractBusinessLogicFromView('EDU_view_Vendas')
  },
  {
    viewName: 'VendasCMV',
    targetService: 'FinancialReportService',
    targetMethod: 'generateCostOfGoodsSoldReport',
    businessLogic: extractBusinessLogicFromView('VendasCMV')
  }
];
```

## Implementation Timeline and Resource Requirements

### **Timeline: 12-Month Migration**

**Months 1-3: Foundation**
- Team: 2 Backend Developers, 1 Frontend Developer, 1 DevOps Engineer
- Focus: Core infrastructure, basic CRUD operations, user management

**Months 4-6: Core Business Logic**
- Team: 3 Backend Developers, 2 Frontend Developers, 1 QA Engineer
- Focus: Sales, inventory, basic financial features

**Months 7-9: Advanced Features**
- Team: 4 Backend Developers, 2 Frontend Developers, 1 QA Engineer, 1 NFe Specialist
- Focus: Brazilian compliance, logistics, advanced reporting

**Months 10-12: Migration and Optimization**
- Team: 2 Backend Developers, 1 Frontend Developer, 1 QA Engineer, 1 Data Migration Specialist
- Focus: Data migration, performance optimization, user training

### **Resource Requirements**

**Development Team:**
- 1 Technical Lead/Architect
- 4 Senior Backend Developers
- 2 Senior Frontend Developers
- 2 QA Engineers
- 1 DevOps Engineer
- 1 NFe/Brazilian Compliance Specialist
- 1 Data Migration Specialist

**Infrastructure:**
- Development, staging, and production environments
- CI/CD pipeline (GitHub Actions or GitLab CI)
- Monitoring and logging (Datadog, New Relic, or ELK stack)
- Database hosting (AWS RDS or similar)
- Container orchestration (Kubernetes or Docker Swarm)

**Total Estimated Cost:** $800K-1.2M over 12 months

## Benefits and Expected Outcomes

### **Technical Benefits**
1. **Improved Maintainability**: Clean architecture enables easier feature development
2. **Better Performance**: Modern stack and optimized queries improve response times
3. **Enhanced Scalability**: Microservices architecture supports horizontal scaling
4. **Improved Testability**: Proper separation of concerns enables comprehensive testing
5. **Better Developer Experience**: Modern tools and practices improve development velocity

### **Business Benefits**
1. **Reduced Development Costs**: Faster feature development and fewer bugs
2. **Improved User Experience**: Modern web interface accessible from any device
3. **Better Business Insights**: Real-time reporting and analytics
4. **Enhanced Compliance**: Robust Brazilian NFe integration
5. **Future-Proof Architecture**: Technology stack that will remain relevant for 10+ years

### **Risk Mitigation**
1. **Gradual Migration**: Phased approach minimizes disruption
2. **Parallel Running**: Old system remains available during transition
3. **Comprehensive Testing**: Automated testing ensures reliability
4. **Expert Team**: Specialized knowledge in Brazilian compliance requirements
5. **Rollback Procedures**: Clear procedures for reverting if issues arise

## Conclusion

The proposed architectural redesign addresses all major issues identified in the current system:

1. **Eliminates Database Complexity**: Business logic moves from 136 views to well-structured application services
2. **Provides Clean Architecture**: Proper separation of concerns with domain-driven design
3. **Enables Scalability**: Microservices architecture supports future growth
4. **Improves Maintainability**: Modern patterns and comprehensive testing
5. **Maintains Brazilian Compliance**: Robust NFe integration with modern libraries

The 12-month migration timeline provides a realistic path forward while minimizing business disruption. The investment of $800K-1.2M will provide a modern, maintainable system that supports business growth for the next decade.

This comprehensive redesign transforms the organically grown system into a well-architected, maintainable, and scalable ERP solution suitable for modern business requirements.