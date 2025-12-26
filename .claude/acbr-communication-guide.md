# ACBr Communication Guide

## Connection Details

- **Server**: localhost (127.0.0.1)
- **Port**: 3434
- **Protocol**: TCP
- **Environment**: Homologação (Test/Staging) - DO NOT CHANGE TO PRODUCTION
- **Version**: ACBrMonitorPLUS Ver. 1.4.0.388 - x86 (IMPORTANT: x64 version has bugs with IBS/CBS)

## Communication Protocol

### Connection Flow
1. Connect to TCP socket on port 3434
2. Wait for welcome message: `ACBrMonitorPLUS Ver. X.X.X.XXX`
3. Send command followed by `\r\n.\r\n` (CRLF + dot + CRLF)
4. Wait for response ending with `\u0003` (ETX character)

### Command Format
```
COMMAND(parameters)\r\n.\r\n
```

### Response Format
- Success: `OK: <response data>`
- Error: Error message without OK prefix
- Alerts: `Alertas: <alert message>`

---

## PowerShell Test Script

Use this script to test ACBr communication from command line:

```powershell
# Test ACBr Connection and Send Command
$command = "NFE.StatusServico()"  # Change this to test different commands

try {
    $client = New-Object System.Net.Sockets.TcpClient('localhost', 3434)
    $stream = $client.GetStream()
    $writer = New-Object System.IO.StreamWriter($stream)
    $reader = New-Object System.IO.StreamReader($stream)

    # Wait for welcome message
    Start-Sleep -Milliseconds 500
    while ($stream.DataAvailable) { $null = $reader.ReadLine() }

    # Send command
    $writer.WriteLine($command)
    $writer.WriteLine('.')
    $writer.Flush()

    # Read response (wait up to 5 seconds)
    Start-Sleep -Milliseconds 3000
    $response = ''
    while ($stream.DataAvailable) {
        $response += $reader.ReadLine() + [Environment]::NewLine
    }

    Write-Output $response
    $client.Close()
} catch {
    Write-Output ('Error: ' + $_.Exception.Message)
}
```

---

## Common ACBr NFe Commands

### Status Commands
```
NFE.StatusServico()                    # Check SEFAZ service status
NFE.Versao()                           # Get ACBr version
```

### NFe Creation & Validation
```
NFE.CriarNFe("<ini_content>",1)        # Create NFe from INI format, return XML
NFE.LoadFromFile(<filepath>)           # Load NFe from file
NFE.SaveToFile(<filepath>, "<xml>")    # Save XML to file
NFE.ValidarNFeRegraNegocios(<filepath>) # Validate business rules (without SEFAZ)
NFE.Validar(<filepath>)                # Validate XML schema
```

### NFe Submission (USE WITH CAUTION - HOMOLOGATION ONLY)
```
NFE.Enviar(<filepath>)                 # Send to SEFAZ
NFE.ConsultarNFe(<filepath>)           # Query NFe status at SEFAZ
NFE.Cancelar(<chave>,<justificativa>)  # Cancel NFe
```

### DANFE (PDF Generation)
```
NFE.ImprimirDANFE(<filepath>)          # Print DANFE
NFE.ImprimirDANFEPDF(<filepath>)       # Generate PDF
```

### Email
```
NFE.EnviarEmail(<email>,<filepath>,1,'<subject>') # Send NFe via email
```

---

## INI Format for NFe Creation

The `NFE.CriarNFe()` command expects an INI-formatted string with sections:

```ini
[Identificacao]
Modelo = 55
Serie = 1
Numero = 123
...

[Emitente]
CNPJ = 12345678000199
...

[Destinatario]
CNPJ = 98765432000111
...

[Produto001]
CFOP = 5102
NCM = 12345678
...

[ICMS001]
CST = 00
...

[PIS001]
CST = 01
...

[COFINS001]
CST = 01
...

# NEW TAX REFORM SECTIONS (2025+)
# IMPORTANT: CST must be 3 digits (000, 010, 200, etc.)
# VERIFIED: Successfully authorized by SEFAZ SP on 2025-12-25
# Tested with ACBr 1.4.0.388 (x86)
# 2026 Test Period Rates: pIBSUF=0.10%, pCBS=0.90%

[IBSCBS001]
CST=000
cClassTrib=000001

[gIBSCBS001]
vBC=100.00
vIBS=0.10

[gIBSUF001]
pIBSUF=0.10
vIBSUF=0.10

[gIBSMun001]
pIBSMun=0.00
vIBSMun=0.00

[gCBS001]
pCBS=0.90
vCBS=0.90

[Total]
BaseICMS=100.00
ValorICMS=18.00
ValorPIS=1.65
ValorCOFINS=7.60
ValorProduto=100.00
ValorNota=100.00

# REQUIRED: IBS/CBS Totals section
[IBSCBSTot]
vBCIBSCBS=100.00

[gIBS]
vIBS=0.10

[gIBSUFTot]
vIBSUF=0.10

[gIBSMunTot]
vIBSMun=0.00

[gCBSTot]
vCBS=0.90

[Transportador]
FretePorConta=9

[Pagamento001]
...

[Volume001]
...

[InfAdic]
...
```

---

## Application Integration (C++)

In the Staccato ERP, ACBr communication is handled by `src/acbr.cpp`:

```cpp
// Key method for sending commands
QString ACBr::enviarComando(const QString &comando, const QString &labelText);

// Example usage in cadastrarnfe.cpp:
ACBr acbr;
QString resposta = acbr.enviarComando(montarXML(), "Gerando NF-e...");
```

### Config Database Fields
- `config.servidorACBr` - Server address (default: localhost)
- `config.portaACBr` - Port number (default: 3434)

---

## Tax Reform 2025 - New Fields

### CST Codes for IBS/CBS (3-digit codes)

| Code | Description |
|------|-------------|
| 000 | Tributação integral (full taxation) |
| 010 | Alíquotas uniformes – FGTS |
| 011 | Alíquotas uniformes |
| 200 | Alíquota reduzida (30-100% reduction) |
| 210 | Redutor de base de cálculo |
| 220 | Alíquota fixa |
| 221 | Alíquota fixa proporcional |
| 400 | Isenção (exempt) |
| 410 | Imunidade e não incidência |
| 510 | Diferimento |
| 550 | Suspensão |
| 620 | Monofásica (fuels) |
| 800 | Transferência de crédito |
| 810 | Ajustes |
| 820 | Regime específico |

**Source**: [Informe Técnico RT 2025.002](https://blog.tecnospeed.com.br/tabela-cclasstrib/)

### IBS (Imposto sobre Bens e Serviços)
- Replaces: ICMS + ISS
- Reference Rate: 17.7%
- Split: State (pIBSUF ~12%) + Municipal (pIBSMun ~5.7%)

### CBS (Contribuição sobre Bens e Serviços)
- Replaces: PIS + COFINS
- Rate: 8.8%

### IS (Imposto Seletivo)
- Replaces: IPI (for specific products)
- Rates vary by product:
  - Cigarettes: 250%
  - Alcoholic beverages: 46-62%
  - Vehicles: ~26.5%
  - Others: 0.25% - 2.5%

---

## Troubleshooting

### Connection Refused
- Verify ACBrMonitorPLUS is running
- Check if port 3434 is correct in config
- Ensure no firewall blocking

### Certificate Errors
- "Erro ao criar a chave do CSP" - Reconnect certificate reader
- "Erro relacionado ao Canal Seguro" - Check SSL/TLS settings

### Validation Errors
- Check XML schema compliance
- Verify all required fields are populated
- Ensure NCM codes are valid

---

## Important Notes

1. **HOMOLOGATION ENVIRONMENT**: Current configuration is for testing only
2. **Do NOT change to production** without proper authorization
3. **All NFes in homologation** are marked as test and not valid for fiscal purposes
4. **Certificate required**: Digital certificate must be installed and accessible
