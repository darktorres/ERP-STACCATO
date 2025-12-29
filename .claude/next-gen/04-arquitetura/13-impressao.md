# Especificações de Impressão

> Status: **Aprovado**
> Última atualização: 2025-12-28

---

## Visão Geral

Este documento define a estratégia de migração do sistema de impressão e geração de documentos do ERP Staccato, convertendo de LimeReport + QtXlsxWriter (C++) para Laravel PDF + Laravel Excel.

### Arquitetura Atual (C++)

```text
┌─────────────────────────────────────────────────────────────┐
│                     Sistema de Impressão                     │
├─────────────────┬─────────────────┬─────────────────────────┤
│   LimeReport    │  QtXlsxWriter   │       ACBr DLL          │
│   (.lrxml)      │    (.xlsx)      │     (DANFE/XML)         │
├─────────────────┼─────────────────┼─────────────────────────┤
│ • Orçamento     │ • Pedido        │ • DANFE NFe             │
│ • Venda         │ • Compras       │                         │
│ • Pallet        │ • Protocolo     │                         │
│ • Relatório NFe │ • Checklist     │                         │
│ • Galpão        │ • Relatórios    │                         │
└─────────────────┴─────────────────┴─────────────────────────┘
```

### Arquitetura Nova (Laravel)

```text
┌─────────────────────────────────────────────────────────────┐
│                     Sistema de Impressão                     │
├─────────────────┬─────────────────┬─────────────────────────┤
│   DomPDF/TCPDF  │  Laravel Excel  │      ACBr REST API      │
│  (Blade → PDF)  │  (Maatwebsite)  │     (Microservice)      │
├─────────────────┼─────────────────┼─────────────────────────┤
│ • Orçamento     │ • Pedido        │ • DANFE NFe             │
│ • Venda         │ • Compras       │ • CC-e                  │
│ • Etiqueta      │ • Protocolo     │ • Manifesto             │
│ • Relatórios    │ • Checklist     │                         │
│ • Romaneio      │ • Exportações   │                         │
└─────────────────┴─────────────────┴─────────────────────────┘
```

---

## Inventário de Documentos

### Documentos PDF (LimeReport → DomPDF)

| Documento | Template Atual | Novo Template | Prioridade |
|-----------|----------------|---------------|------------|
| Orçamento | `orcamento.lrxml` | `pdf.orcamento` | Crítica |
| Venda | `venda.lrxml` | `pdf.venda` | Crítica |
| Etiqueta Pallet | `pallet.lrxml` | `pdf.etiqueta-pallet` | Alta |
| Relatório NFe | `relatorio_nfe.lrxml` | `pdf.relatorio-nfe` | Média |
| Mapa Galpão | `galpao.lrxml` | `pdf.mapa-galpao` | Baixa |

### Documentos Excel (QtXlsx → Laravel Excel)

| Documento | Template Atual | Nova Classe | Prioridade |
|-----------|----------------|-------------|------------|
| Pedido Venda | `pedido.xlsx` | `VendaExport` | Crítica |
| Pedido Compra | `compras.xlsx` | `CompraExport` | Alta |
| Protocolo Entrega | `espelho_entrega.xlsx` | `ProtocoloEntregaExport` | Alta |
| Checklist Entrega | `modelo_checklist.xlsx` | `ChecklistEntregaExport` | Alta |
| Relatório Vendas | `relatorio.xlsx` | `RelatorioVendasExport` | Média |
| Relatório Contábil | `relatorio_contabil.xlsx` | `RelatorioContabilExport` | Média |

### Documentos Fiscais (ACBr)

| Documento | Geração | Estratégia |
|-----------|---------|------------|
| DANFE | ACBrNFe32.dll | ACBr REST API |
| CC-e (Carta Correção) | ACBrNFe32.dll | ACBr REST API |
| Manifesto Destinatário | ACBrNFe32.dll | ACBr REST API |

---

## Stack de Tecnologias

### PDF Generation

```php
// composer.json
{
    "require": {
        "barryvdh/laravel-dompdf": "^3.0",
        "tecnickcom/tcpdf": "^6.7"  // Fallback para PDFs complexos
    }
}
```

**Escolha de Biblioteca:**

| Característica | DomPDF | TCPDF |
|----------------|--------|-------|
| Performance | Rápido | Moderado |
| CSS Support | Excelente | Limitado |
| Unicode/UTF-8 | Excelente | Excelente |
| Tabelas Complexas | Bom | Excelente |
| Gráficos | Não | Sim |
| Uso Recomendado | Documentos simples | Relatórios complexos |

**Decisão:** DomPDF como padrão, TCPDF para relatórios com gráficos ou tabelas muito complexas.

### Excel Generation

```php
// composer.json
{
    "require": {
        "maatwebsite/excel": "^3.1"
    }
}
```

---

## Estrutura de Arquivos

```text
app/
├── Exports/                          # Laravel Excel exports
│   ├── Vendas/
│   │   ├── VendaExport.php
│   │   └── OrcamentoExport.php
│   ├── Compras/
│   │   └── PedidoCompraExport.php
│   ├── Logistica/
│   │   ├── ProtocoloEntregaExport.php
│   │   └── ChecklistExport.php
│   └── Relatorios/
│       ├── VendasMensalExport.php
│       └── ContabilExport.php
│
├── Services/
│   └── Pdf/
│       ├── PdfService.php            # Serviço principal
│       ├── DocumentBuilder.php       # Builder pattern
│       └── Generators/
│           ├── OrcamentoPdfGenerator.php
│           ├── VendaPdfGenerator.php
│           ├── EtiquetaPdfGenerator.php
│           └── RelatorioNfePdfGenerator.php
│
resources/
└── views/
    └── pdf/
        ├── layouts/
        │   ├── documento.blade.php   # Layout base
        │   └── etiqueta.blade.php    # Layout etiqueta
        ├── orcamento.blade.php
        ├── venda.blade.php
        ├── etiqueta-pallet.blade.php
        ├── relatorio-nfe.blade.php
        └── components/
            ├── cabecalho-loja.blade.php
            ├── dados-cliente.blade.php
            ├── tabela-itens.blade.php
            └── rodape-totais.blade.php
```

---

## Implementação PDF

### Serviço Principal

```php
<?php

namespace App\Services\Pdf;

use Barryvdh\DomPDF\Facade\Pdf;
use Illuminate\Support\Facades\Storage;

class PdfService
{
    public function generate(string $view, array $data, array $options = []): string
    {
        $pdf = Pdf::loadView($view, $data);

        $this->configurePdf($pdf, $options);

        return $pdf->output();
    }

    public function download(string $view, array $data, string $filename, array $options = []): \Illuminate\Http\Response
    {
        $pdf = Pdf::loadView($view, $data);

        $this->configurePdf($pdf, $options);

        return $pdf->download($filename);
    }

    public function store(string $view, array $data, string $path, array $options = []): string
    {
        $content = $this->generate($view, $data, $options);

        Storage::disk('documentos')->put($path, $content);

        return $path;
    }

    protected function configurePdf($pdf, array $options): void
    {
        $defaults = [
            'paper' => 'a4',
            'orientation' => 'portrait',
            'margin_top' => 10,
            'margin_right' => 10,
            'margin_bottom' => 10,
            'margin_left' => 10,
        ];

        $config = array_merge($defaults, $options);

        $pdf->setPaper($config['paper'], $config['orientation']);

        $pdf->setOptions([
            'isRemoteEnabled' => true,
            'isHtml5ParserEnabled' => true,
            'defaultFont' => 'sans-serif',
        ]);
    }
}
```

### Generator de Orçamento

```php
<?php

namespace App\Services\Pdf\Generators;

use App\Models\Orcamento;
use App\Services\Pdf\PdfService;

class OrcamentoPdfGenerator
{
    public function __construct(
        private PdfService $pdfService
    ) {}

    public function generate(Orcamento $orcamento): string
    {
        $data = $this->prepareData($orcamento);

        return $this->pdfService->generate('pdf.orcamento', $data, [
            'orientation' => 'landscape',
        ]);
    }

    public function download(Orcamento $orcamento): \Illuminate\Http\Response
    {
        $data = $this->prepareData($orcamento);
        $filename = $this->generateFilename($orcamento);

        return $this->pdfService->download('pdf.orcamento', $data, $filename, [
            'orientation' => 'landscape',
        ]);
    }

    protected function prepareData(Orcamento $orcamento): array
    {
        return [
            'orcamento' => $orcamento->load([
                'cliente.enderecos',
                'loja.endereco',
                'vendedor',
                'profissional',
                'itens.produto',
            ]),
            'loja' => $orcamento->loja,
            'cliente' => $orcamento->cliente,
            'enderecoFiscal' => $orcamento->cliente->enderecoFiscal,
            'enderecoEntrega' => $orcamento->enderecoEntrega,
            'itens' => $orcamento->itens,
            'totais' => $this->calculateTotals($orcamento),
            'validade' => $orcamento->validade->format('d/m/Y'),
            'formaPagamento' => $this->formatPaymentTerms($orcamento),
        ];
    }

    protected function calculateTotals(Orcamento $orcamento): array
    {
        return [
            'subtotal' => $orcamento->itens->sum('total'),
            'desconto' => $orcamento->desconto,
            'frete' => $orcamento->frete,
            'total' => $orcamento->total,
        ];
    }

    protected function formatPaymentTerms(Orcamento $orcamento): string
    {
        return $orcamento->parcelas
            ->map(fn($p) => "{$p->numero}x de R$ " . number_format($p->valor, 2, ',', '.'))
            ->implode(' | ');
    }

    protected function generateFilename(Orcamento $orcamento): string
    {
        $cliente = str_slug($orcamento->cliente->razaoSocial);
        $vendedor = str_slug($orcamento->vendedor->nome);

        return "{$orcamento->id}-{$vendedor}-{$cliente}.pdf";
    }
}
```

### Template Blade - Orçamento

```blade
{{-- resources/views/pdf/orcamento.blade.php --}}
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <title>Orçamento #{{ $orcamento->id }}</title>
    <style>
        @page {
            size: A4 landscape;
            margin: 10mm;
        }

        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: 'Helvetica', 'Arial', sans-serif;
            font-size: 10pt;
            line-height: 1.4;
            color: #333;
        }

        .header {
            display: flex;
            justify-content: space-between;
            border-bottom: 2px solid #2563eb;
            padding-bottom: 10px;
            margin-bottom: 15px;
        }

        .logo {
            max-height: 60px;
        }

        .loja-info {
            text-align: right;
            font-size: 9pt;
        }

        .documento-numero {
            font-size: 18pt;
            font-weight: bold;
            color: #2563eb;
        }

        .section {
            margin-bottom: 15px;
        }

        .section-title {
            font-size: 11pt;
            font-weight: bold;
            color: #1e40af;
            border-bottom: 1px solid #ddd;
            padding-bottom: 3px;
            margin-bottom: 8px;
        }

        .grid-2 {
            display: table;
            width: 100%;
        }

        .grid-2 > div {
            display: table-cell;
            width: 50%;
            vertical-align: top;
            padding-right: 15px;
        }

        .info-row {
            margin-bottom: 3px;
        }

        .info-label {
            font-weight: bold;
            color: #666;
        }

        table.itens {
            width: 100%;
            border-collapse: collapse;
            margin-top: 10px;
        }

        table.itens th {
            background-color: #2563eb;
            color: white;
            padding: 8px 5px;
            text-align: left;
            font-size: 9pt;
        }

        table.itens td {
            padding: 6px 5px;
            border-bottom: 1px solid #eee;
            font-size: 9pt;
        }

        table.itens tr:nth-child(even) {
            background-color: #f9fafb;
        }

        .text-right {
            text-align: right;
        }

        .text-center {
            text-align: center;
        }

        .totais {
            margin-top: 20px;
            float: right;
            width: 250px;
        }

        .totais-row {
            display: flex;
            justify-content: space-between;
            padding: 5px 0;
            border-bottom: 1px solid #eee;
        }

        .totais-row.total {
            font-size: 14pt;
            font-weight: bold;
            color: #2563eb;
            border-top: 2px solid #2563eb;
            border-bottom: none;
        }

        .footer {
            position: fixed;
            bottom: 10mm;
            left: 10mm;
            right: 10mm;
            font-size: 8pt;
            color: #666;
            text-align: center;
            border-top: 1px solid #ddd;
            padding-top: 5px;
        }

        .validade {
            color: #dc2626;
            font-weight: bold;
        }
    </style>
</head>
<body>
    {{-- Cabeçalho --}}
    <div class="header">
        <div>
            @if($loja->logo)
                <img src="{{ storage_path('app/logos/' . $loja->logo) }}" class="logo" alt="{{ $loja->razaoSocial }}">
            @endif
            <div class="documento-numero">ORÇAMENTO #{{ $orcamento->id }}</div>
        </div>
        <div class="loja-info">
            <strong>{{ $loja->razaoSocial }}</strong><br>
            {{ $loja->endereco->logradouro }}, {{ $loja->endereco->numero }}<br>
            {{ $loja->endereco->cidade }}/{{ $loja->endereco->uf }} - CEP {{ $loja->endereco->cep }}<br>
            Tel: {{ $loja->telefone }} | {{ $loja->email }}
        </div>
    </div>

    {{-- Dados do Cliente --}}
    <div class="section">
        <div class="section-title">DADOS DO CLIENTE</div>
        <div class="grid-2">
            <div>
                <div class="info-row">
                    <span class="info-label">Cliente:</span> {{ $cliente->razaoSocial }}
                </div>
                <div class="info-row">
                    <span class="info-label">CPF/CNPJ:</span> {{ $cliente->cpfCnpjFormatado }}
                </div>
                <div class="info-row">
                    <span class="info-label">Email:</span> {{ $cliente->email }}
                </div>
                <div class="info-row">
                    <span class="info-label">Telefone:</span> {{ $cliente->telefone }}
                </div>
            </div>
            <div>
                <div class="info-row">
                    <span class="info-label">Endereço Fiscal:</span><br>
                    {{ $enderecoFiscal->logradouro }}, {{ $enderecoFiscal->numero }}<br>
                    {{ $enderecoFiscal->bairro }} - {{ $enderecoFiscal->cidade }}/{{ $enderecoFiscal->uf }}<br>
                    CEP {{ $enderecoFiscal->cep }}
                </div>
            </div>
        </div>
    </div>

    {{-- Endereço de Entrega (se diferente) --}}
    @if($enderecoEntrega && $enderecoEntrega->id !== $enderecoFiscal->id)
    <div class="section">
        <div class="section-title">ENDEREÇO DE ENTREGA</div>
        <div class="info-row">
            {{ $enderecoEntrega->logradouro }}, {{ $enderecoEntrega->numero }}
            @if($enderecoEntrega->complemento)
                - {{ $enderecoEntrega->complemento }}
            @endif
            <br>
            {{ $enderecoEntrega->bairro }} - {{ $enderecoEntrega->cidade }}/{{ $enderecoEntrega->uf }} - CEP {{ $enderecoEntrega->cep }}
        </div>
    </div>
    @endif

    {{-- Itens --}}
    <div class="section">
        <div class="section-title">ITENS DO ORÇAMENTO</div>
        <table class="itens">
            <thead>
                <tr>
                    <th style="width: 10%">Código</th>
                    <th style="width: 35%">Descrição</th>
                    <th style="width: 15%">Fornecedor</th>
                    <th style="width: 10%" class="text-center">Qtd</th>
                    <th style="width: 8%" class="text-center">Un</th>
                    <th style="width: 11%" class="text-right">Unitário</th>
                    <th style="width: 11%" class="text-right">Total</th>
                </tr>
            </thead>
            <tbody>
                @foreach($itens as $item)
                <tr>
                    <td>{{ $item->produto->codigo }}</td>
                    <td>{{ $item->produto->descricao }}</td>
                    <td>{{ $item->produto->fornecedor->nomeFantasia ?? '-' }}</td>
                    <td class="text-center">{{ number_format($item->quantidade, 2, ',', '.') }}</td>
                    <td class="text-center">{{ $item->unidade }}</td>
                    <td class="text-right">R$ {{ number_format($item->precoUnitario, 2, ',', '.') }}</td>
                    <td class="text-right">R$ {{ number_format($item->total, 2, ',', '.') }}</td>
                </tr>
                @endforeach
            </tbody>
        </table>
    </div>

    {{-- Totais --}}
    <div class="totais">
        <div class="totais-row">
            <span>Subtotal:</span>
            <span>R$ {{ number_format($totais['subtotal'], 2, ',', '.') }}</span>
        </div>
        @if($totais['desconto'] > 0)
        <div class="totais-row">
            <span>Desconto:</span>
            <span>- R$ {{ number_format($totais['desconto'], 2, ',', '.') }}</span>
        </div>
        @endif
        @if($totais['frete'] > 0)
        <div class="totais-row">
            <span>Frete:</span>
            <span>R$ {{ number_format($totais['frete'], 2, ',', '.') }}</span>
        </div>
        @endif
        <div class="totais-row total">
            <span>TOTAL:</span>
            <span>R$ {{ number_format($totais['total'], 2, ',', '.') }}</span>
        </div>
    </div>

    <div style="clear: both;"></div>

    {{-- Condições --}}
    <div class="section" style="margin-top: 30px;">
        <div class="section-title">CONDIÇÕES</div>
        <div class="info-row">
            <span class="info-label">Forma de Pagamento:</span> {{ $formaPagamento }}
        </div>
        <div class="info-row">
            <span class="info-label">Validade:</span>
            <span class="validade">{{ $validade }}</span>
        </div>
        @if($orcamento->observacao)
        <div class="info-row" style="margin-top: 10px;">
            <span class="info-label">Observações:</span><br>
            {{ $orcamento->observacao }}
        </div>
        @endif
    </div>

    {{-- Rodapé --}}
    <div class="footer">
        Documento gerado em {{ now()->format('d/m/Y H:i') }} |
        Vendedor: {{ $orcamento->vendedor->nome }} |
        Este orçamento não tem valor fiscal
    </div>
</body>
</html>
```

### Template Blade - Etiqueta Pallet

```blade
{{-- resources/views/pdf/etiqueta-pallet.blade.php --}}
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <title>Etiqueta Pallet</title>
    <style>
        @page {
            size: A4 landscape;
            margin: 5mm;
        }

        body {
            font-family: 'Helvetica', 'Arial', sans-serif;
            text-align: center;
        }

        .etiqueta {
            border: 3px solid #000;
            padding: 20px;
            margin: 10px;
        }

        .venda-id {
            font-size: 72pt;
            font-weight: bold;
            color: #000;
            margin-bottom: 20px;
        }

        .produto {
            font-size: 36pt;
            font-weight: bold;
            margin-bottom: 15px;
        }

        .info {
            font-size: 24pt;
            margin-bottom: 10px;
        }

        .nfe {
            font-size: 18pt;
            color: #666;
            margin-top: 20px;
        }

        .barcode {
            margin-top: 20px;
        }
    </style>
</head>
<body>
    <div class="etiqueta">
        <div class="venda-id">
            {{ $vendaId ?: 'EST. LOJA' }}
        </div>

        <div class="produto">
            {{ $produto }}
        </div>

        <div class="info">
            <strong>Caixas:</strong> {{ $caixas }}
        </div>

        @if($nfe)
        <div class="nfe">
            NFe: {{ $nfe }}
        </div>
        @endif

        @if($barcode)
        <div class="barcode">
            <img src="data:image/png;base64,{{ $barcode }}" alt="Código de Barras">
        </div>
        @endif
    </div>
</body>
</html>
```

---

## Implementação Excel

### Export de Venda

```php
<?php

namespace App\Exports\Vendas;

use App\Models\Venda;
use Maatwebsite\Excel\Concerns\FromView;
use Maatwebsite\Excel\Concerns\WithStyles;
use Maatwebsite\Excel\Concerns\WithColumnWidths;
use Maatwebsite\Excel\Concerns\WithEvents;
use Maatwebsite\Excel\Events\AfterSheet;
use PhpOffice\PhpSpreadsheet\Worksheet\Worksheet;
use Illuminate\Contracts\View\View;

class VendaExport implements FromView, WithStyles, WithColumnWidths, WithEvents
{
    public function __construct(
        private Venda $venda
    ) {}

    public function view(): View
    {
        return view('exports.venda', [
            'venda' => $this->venda->load([
                'cliente.enderecos',
                'loja.endereco',
                'vendedor',
                'itens.produto',
                'parcelas',
            ]),
        ]);
    }

    public function styles(Worksheet $sheet): array
    {
        return [
            1 => ['font' => ['bold' => true, 'size' => 14]],
            'A1:N1' => ['fill' => [
                'fillType' => \PhpOffice\PhpSpreadsheet\Style\Fill::FILL_SOLID,
                'startColor' => ['rgb' => '2563EB'],
            ]],
        ];
    }

    public function columnWidths(): array
    {
        return [
            'A' => 15,  // Código
            'B' => 40,  // Descrição
            'C' => 20,  // Fornecedor
            'D' => 10,  // Quantidade
            'E' => 8,   // Unidade
            'F' => 15,  // Preço Unitário
            'G' => 15,  // Total
        ];
    }

    public function registerEvents(): array
    {
        return [
            AfterSheet::class => function(AfterSheet $event) {
                $sheet = $event->sheet->getDelegate();

                // Configurar página para impressão
                $sheet->getPageSetup()
                    ->setOrientation(\PhpOffice\PhpSpreadsheet\Worksheet\PageSetup::ORIENTATION_LANDSCAPE)
                    ->setPaperSize(\PhpOffice\PhpSpreadsheet\Worksheet\PageSetup::PAPERSIZE_A4)
                    ->setFitToPage(true)
                    ->setFitToWidth(1)
                    ->setFitToHeight(0);

                // Margens
                $sheet->getPageMargins()
                    ->setTop(0.5)
                    ->setRight(0.5)
                    ->setLeft(0.5)
                    ->setBottom(0.5);
            },
        ];
    }
}
```

### Export de Protocolo de Entrega

```php
<?php

namespace App\Exports\Logistica;

use App\Models\AgendamentoEntrega;
use Maatwebsite\Excel\Concerns\FromView;
use Maatwebsite\Excel\Concerns\WithStyles;
use Maatwebsite\Excel\Concerns\WithEvents;
use Maatwebsite\Excel\Events\AfterSheet;
use PhpOffice\PhpSpreadsheet\Worksheet\Worksheet;
use Illuminate\Contracts\View\View;

class ProtocoloEntregaExport implements FromView, WithStyles, WithEvents
{
    public function __construct(
        private AgendamentoEntrega $agendamento
    ) {}

    public function view(): View
    {
        return view('exports.protocolo-entrega', [
            'agendamento' => $this->agendamento->load([
                'venda.cliente',
                'venda.itens.produto',
                'venda.itens.estoque',
                'motorista',
                'veiculo',
            ]),
        ]);
    }

    public function styles(Worksheet $sheet): array
    {
        return [
            'A1:Z1' => ['font' => ['bold' => true]],
        ];
    }

    public function registerEvents(): array
    {
        return [
            AfterSheet::class => function(AfterSheet $event) {
                $sheet = $event->sheet->getDelegate();

                // Orientação vertical para protocolo
                $sheet->getPageSetup()
                    ->setOrientation(\PhpOffice\PhpSpreadsheet\Worksheet\PageSetup::ORIENTATION_PORTRAIT)
                    ->setPaperSize(\PhpOffice\PhpSpreadsheet\Worksheet\PageSetup::PAPERSIZE_A4);
            },
        ];
    }
}
```

### Template Blade para Excel

```blade
{{-- resources/views/exports/venda.blade.php --}}
<table>
    {{-- Cabeçalho da Loja --}}
    <tr>
        <td colspan="7" style="font-size: 14pt; font-weight: bold;">
            {{ $venda->loja->razaoSocial }}
        </td>
    </tr>
    <tr>
        <td colspan="7">
            {{ $venda->loja->endereco->logradouro }}, {{ $venda->loja->endereco->numero }} -
            {{ $venda->loja->endereco->cidade }}/{{ $venda->loja->endereco->uf }}
        </td>
    </tr>
    <tr><td colspan="7"></td></tr>

    {{-- Número do Pedido --}}
    <tr>
        <td colspan="7" style="font-size: 16pt; font-weight: bold; color: #2563EB;">
            PEDIDO DE VENDA #{{ $venda->id }}
        </td>
    </tr>
    <tr><td colspan="7"></td></tr>

    {{-- Dados do Cliente --}}
    <tr>
        <td style="font-weight: bold;">Cliente:</td>
        <td colspan="6">{{ $venda->cliente->razaoSocial }}</td>
    </tr>
    <tr>
        <td style="font-weight: bold;">CPF/CNPJ:</td>
        <td colspan="6">{{ $venda->cliente->cpfCnpjFormatado }}</td>
    </tr>
    <tr>
        <td style="font-weight: bold;">Email:</td>
        <td colspan="6">{{ $venda->cliente->email }}</td>
    </tr>
    <tr><td colspan="7"></td></tr>

    {{-- Cabeçalho dos Itens --}}
    <tr style="background-color: #2563EB; color: white; font-weight: bold;">
        <td>Código</td>
        <td>Descrição</td>
        <td>Fornecedor</td>
        <td>Qtd</td>
        <td>Un</td>
        <td>Unit.</td>
        <td>Total</td>
    </tr>

    {{-- Itens --}}
    @foreach($venda->itens as $item)
    <tr>
        <td>{{ $item->produto->codigo }}</td>
        <td>{{ $item->produto->descricao }}</td>
        <td>{{ $item->produto->fornecedor->nomeFantasia ?? '-' }}</td>
        <td>{{ number_format($item->quantidade, 2, ',', '.') }}</td>
        <td>{{ $item->unidade }}</td>
        <td>{{ number_format($item->precoUnitario, 2, ',', '.') }}</td>
        <td>{{ number_format($item->total, 2, ',', '.') }}</td>
    </tr>
    @endforeach

    <tr><td colspan="7"></td></tr>

    {{-- Totais --}}
    <tr>
        <td colspan="5"></td>
        <td style="font-weight: bold;">Subtotal:</td>
        <td>R$ {{ number_format($venda->subtotal, 2, ',', '.') }}</td>
    </tr>
    @if($venda->desconto > 0)
    <tr>
        <td colspan="5"></td>
        <td style="font-weight: bold;">Desconto:</td>
        <td>- R$ {{ number_format($venda->desconto, 2, ',', '.') }}</td>
    </tr>
    @endif
    @if($venda->frete > 0)
    <tr>
        <td colspan="5"></td>
        <td style="font-weight: bold;">Frete:</td>
        <td>R$ {{ number_format($venda->frete, 2, ',', '.') }}</td>
    </tr>
    @endif
    <tr>
        <td colspan="5"></td>
        <td style="font-weight: bold; font-size: 12pt;">TOTAL:</td>
        <td style="font-weight: bold; font-size: 12pt;">R$ {{ number_format($venda->total, 2, ',', '.') }}</td>
    </tr>

    <tr><td colspan="7"></td></tr>

    {{-- Parcelas --}}
    <tr>
        <td colspan="7" style="font-weight: bold;">Forma de Pagamento:</td>
    </tr>
    @foreach($venda->parcelas as $parcela)
    <tr>
        <td colspan="7">
            {{ $parcela->numero }}ª Parcela: R$ {{ number_format($parcela->valor, 2, ',', '.') }}
            - Vencimento: {{ $parcela->vencimento->format('d/m/Y') }}
        </td>
    </tr>
    @endforeach
</table>
```

---

## Integração DANFE (ACBr)

### ACBr REST API Service

```php
<?php

namespace App\Services\Fiscal;

use Illuminate\Support\Facades\Http;
use Illuminate\Support\Facades\Storage;

class AcbrService
{
    private string $baseUrl;

    public function __construct()
    {
        $this->baseUrl = config('services.acbr.url');
    }

    public function gerarDanfe(string $xml): string
    {
        $response = Http::timeout(30)
            ->post("{$this->baseUrl}/nfe/danfe", [
                'xml' => $xml,
                'formato' => 'pdf',
            ]);

        if (!$response->successful()) {
            throw new \Exception('Erro ao gerar DANFE: ' . $response->body());
        }

        return base64_decode($response->json('pdf'));
    }

    public function gerarDanfeFromChave(string $chaveAcesso): string
    {
        $response = Http::timeout(30)
            ->get("{$this->baseUrl}/nfe/danfe/{$chaveAcesso}");

        if (!$response->successful()) {
            throw new \Exception('Erro ao gerar DANFE: ' . $response->body());
        }

        return base64_decode($response->json('pdf'));
    }

    public function downloadDanfe(string $chaveAcesso): \Illuminate\Http\Response
    {
        $pdf = $this->gerarDanfeFromChave($chaveAcesso);

        return response($pdf, 200, [
            'Content-Type' => 'application/pdf',
            'Content-Disposition' => "attachment; filename=\"{$chaveAcesso}-danfe.pdf\"",
        ]);
    }

    public function storeDanfe(string $chaveAcesso, string $xml): string
    {
        $pdf = $this->gerarDanfe($xml);

        $path = "danfe/{$chaveAcesso}.pdf";
        Storage::disk('documentos')->put($path, $pdf);

        return $path;
    }
}
```

### Configuração ACBr

```php
// config/services.php
return [
    'acbr' => [
        'url' => env('ACBR_API_URL', 'http://localhost:8080'),
        'timeout' => env('ACBR_TIMEOUT', 30),
    ],
];
```

---

## API de Documentos

### Controller de PDFs

```php
<?php

namespace App\Http\Controllers\Api;

use App\Http\Controllers\Controller;
use App\Models\Orcamento;
use App\Models\Venda;
use App\Services\Pdf\Generators\OrcamentoPdfGenerator;
use App\Services\Pdf\Generators\VendaPdfGenerator;
use Illuminate\Http\Request;

class DocumentoController extends Controller
{
    public function orcamentoPdf(Orcamento $orcamento, OrcamentoPdfGenerator $generator)
    {
        $this->authorize('view', $orcamento);

        return $generator->download($orcamento);
    }

    public function vendaPdf(Venda $venda, VendaPdfGenerator $generator)
    {
        $this->authorize('view', $venda);

        return $generator->download($venda);
    }

    public function orcamentoPreview(Orcamento $orcamento, OrcamentoPdfGenerator $generator)
    {
        $this->authorize('view', $orcamento);

        $pdf = $generator->generate($orcamento);

        return response($pdf, 200, [
            'Content-Type' => 'application/pdf',
            'Content-Disposition' => 'inline',
        ]);
    }
}
```

### Rotas

```php
// routes/api.php
Route::prefix('documentos')->group(function () {
    // PDFs
    Route::get('orcamentos/{orcamento}/pdf', [DocumentoController::class, 'orcamentoPdf']);
    Route::get('orcamentos/{orcamento}/preview', [DocumentoController::class, 'orcamentoPreview']);
    Route::get('vendas/{venda}/pdf', [DocumentoController::class, 'vendaPdf']);
    Route::get('vendas/{venda}/preview', [DocumentoController::class, 'vendaPreview']);

    // Excel
    Route::get('orcamentos/{orcamento}/excel', [DocumentoController::class, 'orcamentoExcel']);
    Route::get('vendas/{venda}/excel', [DocumentoController::class, 'vendaExcel']);
    Route::get('compras/{compra}/excel', [DocumentoController::class, 'compraExcel']);

    // DANFE
    Route::get('nfe/{nfe}/danfe', [DocumentoController::class, 'danfe']);
    Route::get('nfe/{nfe}/xml', [DocumentoController::class, 'xml']);

    // Logística
    Route::get('entregas/{agendamento}/protocolo', [DocumentoController::class, 'protocoloEntrega']);
    Route::get('entregas/{agendamento}/checklist', [DocumentoController::class, 'checklistEntrega']);

    // Etiquetas
    Route::get('estoque/{estoque}/etiqueta', [DocumentoController::class, 'etiquetaPallet']);
    Route::post('etiquetas/lote', [DocumentoController::class, 'etiquetasLote']);
});
```

---

## Configuração de Usuário

### Model UserPreference

```php
<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class UserPreference extends Model
{
    protected $fillable = [
        'user_id',
        'key',
        'value',
    ];

    protected $casts = [
        'value' => 'json',
    ];
}

// Uso
User::find(1)->getPreference('documentos.pasta_orcamentos', storage_path('app/orcamentos'));
User::find(1)->setPreference('documentos.pasta_orcamentos', '/custom/path');
```

### Preferências de Impressão

```php
<?php

namespace App\Settings;

class DocumentoSettings
{
    public function __construct(
        public string $pastaOrcamentos = 'orcamentos',
        public string $pastaVendas = 'vendas',
        public string $pastaCompras = 'compras',
        public string $pastaEntregas = 'entregas',
        public string $pastaDanfe = 'danfe',
        public string $formatoPadrao = 'pdf',
        public bool $abrirAposGerar = true,
        public string $orientacaoPadrao = 'landscape',
        public string $tamanhoPapel = 'a4',
    ) {}
}
```

---

## Geração Assíncrona (Queue)

### Job de Geração de PDF

```php
<?php

namespace App\Jobs;

use App\Models\Venda;
use App\Notifications\DocumentoGerado;
use App\Services\Pdf\Generators\VendaPdfGenerator;
use Illuminate\Bus\Queueable;
use Illuminate\Contracts\Queue\ShouldQueue;
use Illuminate\Foundation\Bus\Dispatchable;
use Illuminate\Queue\InteractsWithQueue;
use Illuminate\Queue\SerializesModels;
use Illuminate\Support\Facades\Storage;

class GerarVendaPdfJob implements ShouldQueue
{
    use Dispatchable, InteractsWithQueue, Queueable, SerializesModels;

    public function __construct(
        public Venda $venda,
        public int $userId
    ) {}

    public function handle(VendaPdfGenerator $generator): void
    {
        $pdf = $generator->generate($this->venda);

        $filename = $this->generateFilename();
        Storage::disk('documentos')->put("vendas/{$filename}", $pdf);

        // Notificar usuário
        $user = \App\Models\User::find($this->userId);
        $user->notify(new DocumentoGerado('Venda', $this->venda->id, "vendas/{$filename}"));
    }

    protected function generateFilename(): string
    {
        $timestamp = now()->format('Ymd_His');
        return "{$this->venda->id}_{$timestamp}.pdf";
    }
}
```

### Uso do Job

```php
// Geração síncrona (resposta imediata)
return $generator->download($venda);

// Geração assíncrona (para relatórios grandes)
GerarVendaPdfJob::dispatch($venda, auth()->id());
return response()->json(['message' => 'Documento será gerado em breve']);
```

---

## Especificações de Papel

### Tamanhos Suportados

| Tipo | Tamanho | Uso |
|------|---------|-----|
| A4 Portrait | 210 x 297 mm | Relatórios, protocolos |
| A4 Landscape | 297 x 210 mm | Orçamentos, vendas, pedidos |
| A5 | 148 x 210 mm | Etiquetas pequenas |
| Custom Label | 100 x 150 mm | Etiquetas de pallet |
| Thermal 80mm | 80 x contínuo | Cupons (futuro) |

### Margens Padrão

```php
// config/dompdf.php
return [
    'default_paper_size' => 'a4',
    'default_paper_orientation' => 'landscape',
    'default_margin' => [
        'top' => 10,    // mm
        'right' => 10,
        'bottom' => 10,
        'left' => 10,
    ],
];
```

---

## Frontend (Vue)

### Composable de Documentos

```typescript
// composables/useDocumentos.ts
import { ref } from 'vue'
import axios from 'axios'

export function useDocumentos() {
    const loading = ref(false)
    const error = ref<string | null>(null)

    async function downloadPdf(tipo: string, id: number) {
        loading.value = true
        error.value = null

        try {
            const response = await axios.get(`/api/documentos/${tipo}/${id}/pdf`, {
                responseType: 'blob',
            })

            const url = window.URL.createObjectURL(response.data)
            const link = document.createElement('a')
            link.href = url
            link.download = `${tipo}-${id}.pdf`
            link.click()

            window.URL.revokeObjectURL(url)
        } catch (e) {
            error.value = 'Erro ao gerar documento'
            throw e
        } finally {
            loading.value = false
        }
    }

    async function previewPdf(tipo: string, id: number) {
        loading.value = true
        error.value = null

        try {
            const response = await axios.get(`/api/documentos/${tipo}/${id}/preview`, {
                responseType: 'blob',
            })

            const url = window.URL.createObjectURL(response.data)
            window.open(url, '_blank')
        } catch (e) {
            error.value = 'Erro ao visualizar documento'
            throw e
        } finally {
            loading.value = false
        }
    }

    async function downloadExcel(tipo: string, id: number) {
        loading.value = true
        error.value = null

        try {
            const response = await axios.get(`/api/documentos/${tipo}/${id}/excel`, {
                responseType: 'blob',
            })

            const url = window.URL.createObjectURL(response.data)
            const link = document.createElement('a')
            link.href = url
            link.download = `${tipo}-${id}.xlsx`
            link.click()

            window.URL.revokeObjectURL(url)
        } catch (e) {
            error.value = 'Erro ao gerar planilha'
            throw e
        } finally {
            loading.value = false
        }
    }

    return {
        loading,
        error,
        downloadPdf,
        previewPdf,
        downloadExcel,
    }
}
```

### Componente de Ações de Documento

```vue
<!-- components/DocumentActions.vue -->
<template>
    <div class="flex gap-2">
        <Button
            icon="pi pi-file-pdf"
            label="PDF"
            severity="danger"
            :loading="loading"
            @click="handlePdf"
        />
        <Button
            icon="pi pi-file-excel"
            label="Excel"
            severity="success"
            :loading="loading"
            @click="handleExcel"
        />
        <Button
            icon="pi pi-eye"
            label="Visualizar"
            outlined
            :loading="loading"
            @click="handlePreview"
        />
    </div>
</template>

<script setup lang="ts">
import { useDocumentos } from '@/composables/useDocumentos'
import Button from 'primevue/button'

const props = defineProps<{
    tipo: 'orcamentos' | 'vendas' | 'compras'
    id: number
}>()

const { loading, downloadPdf, downloadExcel, previewPdf } = useDocumentos()

function handlePdf() {
    downloadPdf(props.tipo, props.id)
}

function handleExcel() {
    downloadExcel(props.tipo, props.id)
}

function handlePreview() {
    previewPdf(props.tipo, props.id)
}
</script>
```

---

## Migração de Templates

### Checklist de Conversão

| Template Original | Template Novo | Status |
|-------------------|---------------|--------|
| `orcamento.lrxml` | `pdf/orcamento.blade.php` | Pendente |
| `venda.lrxml` | `pdf/venda.blade.php` | Pendente |
| `pallet.lrxml` | `pdf/etiqueta-pallet.blade.php` | Pendente |
| `relatorio_nfe.lrxml` | `pdf/relatorio-nfe.blade.php` | Pendente |
| `galpao.lrxml` | `pdf/mapa-galpao.blade.php` | Pendente |
| `pedido.xlsx` | `VendaExport.php` | Pendente |
| `compras.xlsx` | `CompraExport.php` | Pendente |
| `espelho_entrega.xlsx` | `ProtocoloEntregaExport.php` | Pendente |
| `modelo_checklist.xlsx` | `ChecklistExport.php` | Pendente |

### Processo de Conversão

1. **Analisar template original** - Identificar campos, layout, estilos
2. **Criar Blade template** - Converter para HTML/CSS
3. **Criar Generator/Export** - Implementar lógica de dados
4. **Testar visualmente** - Comparar com original
5. **Validar com usuários** - Aprovação do layout
6. **Deploy e monitorar** - Verificar erros em produção

---

## Documentos Relacionados

- [06-api.md](./06-api.md) - Design de API (endpoints de documentos)
- [11-concorrencia.md](./11-concorrencia.md) - Concorrência (geração paralela)
- [../estrategia/11-treinamento.md](../estrategia/11-treinamento.md) - Treinamento (como imprimir)
