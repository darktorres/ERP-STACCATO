# ERP Staccato - Process Flowchart & Architecture Documentation

## System Overview

ERP Staccato is a comprehensive Brazilian business management system built with Qt C++ and MySQL/MariaDB, specifically designed for Brazilian fiscal compliance including NFe (Electronic Invoices) and tax management.

## High-Level Architecture

```mermaid
graph TB
    subgraph "User Interface Layer"
        MW[MainWindow] --> TC[TabCompras]
        MW --> TE[TabEstoque]
        MW --> TF[TabFinanceiro]
        MW --> TL[TabLogistica]
        MW --> TN[TabNFe]
    end
    
    subgraph "Application Layer"
        APP[Application Class]
        APP --> DB[(MySQL Database)]
        APP --> TR[Transaction Management]
        APP --> EH[Exception Handling]
    end
    
    subgraph "External Systems"
        ACBr[ACBr Library<br/>Brazilian Fiscal]
        LR[LimeReport<br/>Report Generation]
        SSL[OpenSSL<br/>Security]
    end
    
    MW --> APP
    APP --> ACBr
    APP --> LR
    APP --> SSL
```

## Core Business Process Flow

### 1. Sales Process Flow

```mermaid
flowchart TD
    Start([Customer Inquiry]) --> Orc[Create Orcamento<br/>Quote/Budget]
    
    Orc --> OrcDetails{Quote Details}
    OrcDetails --> AddProd[Add Products<br/>Calculate Prices]
    AddProd --> CalcFrete[Calculate Freight<br/>Define Payment Terms]
    CalcFrete --> SaveOrc[Save Quote]
    
    SaveOrc --> Decision{Customer Decision}
    Decision -->|Accept| ConvertSale[Convert to Venda<br/>Sales Order]
    Decision -->|Reject| CloseOrc[Close Quote<br/>Status: PERDIDO]
    Decision -->|Pending| FollowUp[Follow-up Process]
    
    ConvertSale --> VendaSave[Save Sales Order<br/>Status: Various Logistics States]
    VendaSave --> GenPedido[Generate Purchase Orders<br/>for Missing Products]
    
    GenPedido --> LogisticaFlow[Logistics Processing]
    LogisticaFlow --> Entrega[Delivery]
    Entrega --> FinanceiroFlow[Financial Processing]
    
    FollowUp --> Decision
    
    classDef process fill:#e1f5fe
    classDef decision fill:#fff3e0
    classDef endpoint fill:#e8f5e8
    
    class Orc,AddProd,CalcFrete,SaveOrc,ConvertSale,VendaSave,GenPedido process
    class Decision,OrcDetails decision
    class Start,CloseOrc,Entrega endpoint
```

### 2. Purchase Process Flow

```mermaid
flowchart TD
    VendaNeed[Sales Order<br/>Needs Products] --> CheckEst{Check Inventory}
    CheckEst -->|In Stock| AllocateEst[Allocate from<br/>Existing Stock]
    CheckEst -->|Out of Stock| PendenteProd[Mark Product<br/>Status: PENDENTE]
    
    PendenteProd --> WidgetPend[WidgetCompraPendentes<br/>Pending Products View]
    WidgetPend --> IniciarCompra[Initiate Purchase<br/>Status: INICIADO]
    
    IniciarCompra --> WidgetGerar[WidgetCompraGerar<br/>Generate Purchase Orders]
    WidgetGerar --> GroupBySupp[Group Products<br/>by Supplier]
    GroupBySupp --> GenExcel[Generate Excel Files<br/>for Suppliers]
    GenExcel --> SendEmail[Send PO via Email<br/>Status: EM COMPRA]
    
    SendEmail --> WidgetConf[WidgetCompraConfirmar<br/>Confirm Orders]
    WidgetConf --> SupplierConf[Supplier Confirms<br/>Status: EM FATURAMENTO]
    
    SupplierConf --> WidgetFat[WidgetCompraFaturar<br/>Invoice Processing]
    WidgetFat --> InvoiceRec[Receive Invoice<br/>Status: EM ENTREGA]
    
    InvoiceRec --> EstoqueRec[Receive in Inventory<br/>Status: ESTOQUE]
    EstoqueRec --> AllocateEst
    
    AllocateEst --> CreateConsumo[Create Consumption<br/>Record]
    CreateConsumo --> UpdateSalesStatus[Update Sales Order<br/>Status]
    
    classDef process fill:#e1f5fe
    classDef decision fill:#fff3e0
    classDef status fill:#f3e5f5
    
    class WidgetPend,WidgetGerar,WidgetConf,WidgetFat process
    class CheckEst decision
    class PendenteProd,IniciarCompra,SendEmail,SupplierConf,InvoiceRec,EstoqueRec status
```

### 3. Inventory Management Flow

```mermaid
flowchart TD
    ProdArrival[Product Arrival] --> CreateEst[Create Estoque Record]
    CreateEst --> LinkPO[Link to Purchase Order<br/>estoque_has_compra]
    
    LinkPO --> StockReady[Stock Available<br/>Status: Green/OK]
    
    SalesNeed[Sales Order<br/>Needs Product] --> CheckAvail{Check Availability}
    CheckAvail -->|Available| CreateCons[Create Consumption<br/>criarConsumo()]
    CheckAvail -->|Insufficient| PartialCons[Partial Consumption<br/>Split Order]
    CheckAvail -->|Not Available| BackOrder[Trigger Purchase<br/>Process]
    
    CreateCons --> UpdateQuant[Update Remaining<br/>Quantities]
    UpdateQuant --> LinkSales[Link to Sales Order<br/>idVendaProduto2]
    
    PartialCons --> DivideOrder[dividirCompra()<br/>Split Purchase Order]
    DivideOrder --> CreateCons
    
    LinkSales --> StatusUpdate[Update Sales Status<br/>Ready for Logistics]
    
    ManualAdj[Manual Adjustment] --> AdjustQuant[on_pushButtonAjustarQuant_clicked()]
    AdjustQuant --> CreateAdjRec[Create Adjustment<br/>Record]
    
    BackOrder --> PurchaseFlow[Purchase Process<br/>Flow]
    
    classDef inventory fill:#e8f5e8
    classDef process fill:#e1f5fe
    classDef decision fill:#fff3e0
    
    class CreateEst,CreateCons,UpdateQuant,LinkSales inventory
    class CheckAvail decision
    class AdjustQuant,DivideOrder process
```

### 4. Financial Integration Flow

```mermaid
flowchart TD
    SalesOrder[Sales Order<br/>Created] --> GenReceivables[Generate Accounts<br/>Receivable]
    
    PurchaseOrder[Purchase Order<br/>Invoiced] --> GenPayables[Generate Accounts<br/>Payable]
    
    GenReceivables --> PaymentTerms[Apply Payment<br/>Terms]
    GenPayables --> SupplierTerms[Apply Supplier<br/>Terms]
    
    PaymentTerms --> InstallmentPlan[Create Installment<br/>Plan]
    SupplierTerms --> PaymentSched[Payment Schedule]
    
    InstallmentPlan --> TrackReceivables[Track Receivables<br/>WidgetFinanceiroContas]
    PaymentSched --> TrackPayables[Track Payables<br/>conta_a_pagar]
    
    TrackReceivables --> CollectionProcess[Collection Process]
    TrackPayables --> PaymentProcess[Payment Process]
    
    CollectionProcess --> CashFlow[Cash Flow<br/>Management]
    PaymentProcess --> CashFlow
    
    CashFlow --> FinancialReports[Financial Reports<br/>LimeReport]
    
    classDef financial fill:#fff8e1
    classDef process fill:#e1f5fe
    
    class GenReceivables,GenPayables,PaymentTerms,SupplierTerms financial
    class InstallmentPlan,PaymentSched,CollectionProcess,PaymentProcess process
```

### 5. Logistics & Delivery Flow

```mermaid
flowchart TD
    SalesReady[Sales Order<br/>Ready for Delivery] --> LogisticsPlan[Logistics Planning<br/>TabLogistica]
    
    LogisticsPlan --> ScheduleCol[Schedule Collection<br/>WidgetLogisticaAgendarColeta]
    ScheduleCol --> ScheduleEnt[Schedule Delivery<br/>WidgetLogisticaAgendarEntrega]
    
    ScheduleEnt --> Calendar[Logistics Calendar<br/>WidgetLogisticaCalendario]
    Calendar --> TruckPlan[Truck Planning<br/>WidgetLogisticaCaminhao]
    
    TruckPlan --> Coleta[Collection Process<br/>WidgetLogisticaColeta]
    Coleta --> Transit[In Transit<br/>Status: EM ENTREGA]
    
    Transit --> Delivery[Delivery Process<br/>WidgetLogisticaEntregas]
    Delivery --> Delivered[Delivered<br/>Status: ENTREGUE]
    
    Delivered --> UpdateSales[Update Sales Status]
    UpdateSales --> TriggerInvoice[Trigger Invoice<br/>Generation]
    
    TriggerInvoice --> NFe[Generate NFe<br/>Electronic Invoice]
    NFe --> SendNFe[Send NFe to<br/>Customer & Government]
    
    classDef logistics fill:#e3f2fd
    classDef nfe fill:#f1f8e9
    classDef process fill:#e1f5fe
    
    class ScheduleCol,ScheduleEnt,Calendar,TruckPlan,Coleta,Delivery logistics
    class NFe,SendNFe nfe
    class LogisticsPlan,UpdateSales,TriggerInvoice process
```

### 6. NFe (Electronic Invoice) Process

```mermaid
flowchart TD
    DeliveryConf[Delivery Confirmed] --> PrepareNFe[Prepare NFe Data<br/>WidgetNFeSaida]
    
    PrepareNFe --> ProductList[Product List<br/>with Tax Info]
    ProductList --> TaxCalc[Calculate Taxes<br/>ICMS, IPI, PIS, COFINS]
    
    TaxCalc --> ACBrInteg[ACBr Integration<br/>Brazilian Fiscal Library]
    ACBrInteg --> GenXML[Generate NFe XML]
    
    GenXML --> SignXML[Digital Signature<br/>A1/A3 Certificate]
    SignXML --> ValidateXML[Validate XML<br/>Schema & Business Rules]
    
    ValidateXML --> SendSEFAZ[Send to SEFAZ<br/>Government System]
    SendSEFAZ --> AuthResp{Authorization<br/>Response}
    
    AuthResp -->|Authorized| StoreNFe[Store NFe Number<br/>& Access Key]
    AuthResp -->|Rejected| FixErrors[Fix Errors<br/>& Retry]
    
    StoreNFe --> GenDANFE[Generate DANFE<br/>PDF Report]
    GenDANFE --> SendCustomer[Send DANFE &<br/>XML to Customer]
    
    FixErrors --> PrepareNFe
    
    classDef nfe fill:#f1f8e9
    classDef process fill:#e1f5fe
    classDef decision fill:#fff3e0
    classDef external fill:#ffebee
    
    class PrepareNFe,ProductList,TaxCalc,GenXML,SignXML,ValidateXML nfe
    class AuthResp decision
    class ACBrInteg,SendSEFAZ external
```

## Key Integration Points

### Database Architecture
- **Core Tables**: `loja`, `produto`, `cliente`, `fornecedor`
- **Sales Tables**: `orcamento`, `venda`, `venda_has_produto2`
- **Purchase Tables**: `pedido_fornecedor_has_produto2`, `estoque_has_compra`
- **Inventory Tables**: `estoque`, `estoque_has_consumo`
- **Financial Tables**: `conta_a_pagar`, `conta_a_receber`
- **NFe Tables**: `nfe`, `nfe_has_produto`

### Status Flow Summary

#### Sales Order Status:
```
Quote: ATIVO → FECHADO/PERDIDO/CANCELADO
Sales: Various logistics states (EM COLETA, EM RECEBIMENTO, EM ENTREGA, ENTREGUE)
```

#### Purchase Order Status:
```
PENDENTE → EM COMPRA → EM FATURAMENTO → EM ENTREGA → ESTOQUE
```

#### Inventory Status:
```
Color-coded: White (Unprocessed) → Green (OK) → Yellow (Quantity differs) → Red (Not found)
Consumption: TEMP → CONSUMO → AJUSTE/CANCELADO
```

### Brazilian Compliance Features
- **NFe Integration**: Full electronic invoice generation and transmission
- **Tax Calculation**: Automatic ICMS, IPI, PIS, COFINS calculations
- **SEFAZ Integration**: Direct communication with government systems
- **DANFE Reports**: Official invoice PDF generation
- **Fiscal Certificates**: A1/A3 digital certificate support

### External Dependencies
- **ACBr Library**: Brazilian fiscal compliance
- **LimeReport**: Report generation and design
- **QtXlsxWriter**: Excel file generation
- **OpenSSL**: Security and certificates
- **cURL**: HTTP communications

This documentation provides a comprehensive overview of the organically grown ERP system, showing how the various modules interconnect and process business transactions from initial quote through final delivery and invoicing.