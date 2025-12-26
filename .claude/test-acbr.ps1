# ACBr NFe Test Script - Incremental Stages
# Usage: .\test-acbr.ps1 [stage]
# Stages:
#   0 - Test connection only (NFE.Versao)
#   1 - Minimal NFe (Identificacao + Emitente + Destinatario only)
#   2 - Add one product with ICMS
#   3 - Complete NFe with ICMS + PIS + COFINS
#   4 - NFe with IBS/CBS (Tax Reform 2025)
#   5 - Full NFe from app debug file
#   6 - Create + Validate NFe
#   7 - Create + Validate + Send to SEFAZ (HOMOLOGATION)
#   8 - Test NomeArq behavior (verify authorized NFe path in response)

param(
    [int]$Stage = 0
)

$host.UI.RawUI.WindowTitle = "ACBr NFe Test - Stage $Stage"

function Send-ACBrCommand {
    param(
        [string]$Command,
        [int]$TimeoutSec = 30
    )

    try {
        $client = [System.Net.Sockets.TcpClient]::new('localhost', 3434)
        $stream = $client.GetStream()
        $stream.ReadTimeout = $TimeoutSec * 1000
        $buffer = New-Object byte[] 65536

        # Read welcome
        Start-Sleep -Milliseconds 500
        while ($stream.DataAvailable) {
            $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
            $welcome = [System.Text.Encoding]::UTF8.GetString($buffer, 0, $bytesRead)
            Write-Host "Connected: $($welcome.Trim())" -ForegroundColor Green
        }

        # Send command
        $cmdBytes = [System.Text.Encoding]::UTF8.GetBytes($Command + "`r`n.`r`n")
        $stream.Write($cmdBytes, 0, $cmdBytes.Length)
        $stream.Flush()
        Write-Host "Sent: $($cmdBytes.Length) bytes" -ForegroundColor Gray

        # Read response
        $response = ""
        $timeout = [DateTime]::Now.AddSeconds($TimeoutSec)
        while ([DateTime]::Now -lt $timeout) {
            if ($stream.DataAvailable) {
                $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
                $chunk = [System.Text.Encoding]::UTF8.GetString($buffer, 0, $bytesRead)
                $response += $chunk
                if ($response.IndexOf([char]0x03) -ge 0) {
                    $client.Close()
                    return $response.TrimEnd([char]0x03)
                }
            } else {
                Start-Sleep -Milliseconds 100
            }
        }
        $client.Close()
        return $response
    } catch {
        Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
        return $null
    }
}

Write-Host "`n=== ACBr NFe Test - Stage $Stage ===" -ForegroundColor Cyan

switch ($Stage) {
    0 {
        Write-Host "Testing connection with NFE.Versao()..." -ForegroundColor Yellow
        $resp = Send-ACBrCommand "NFE.Versao()"
        Write-Host "`nResponse:`n$resp" -ForegroundColor White
    }

    1 {
        Write-Host "Minimal NFe - Identificacao + Emitente + Destinatario only" -ForegroundColor Yellow
        $ini = @"
[Identificacao]
NaturezaOperacao=VENDA
Modelo=55
Serie=001
Codigo=12121212
Numero=999999
Emissao=25/12/2025
Saida=25/12/2025
Tipo=1
finNFe=1
FormaPag=0
idDest=1
indPres=1
indFinal=1
[Emitente]
CNPJ=09375013000110
IE=206257840114
Razao=STACCATO REVESTIMENTOS
Fantasia=STACCATO
Fone=1141919816
CEP=06455000
Logradouro=Alameda Araguaia
Numero=661
Bairro=Alphaville
cMun=3505708
Cidade=Barueri
UF=SP
[Destinatario]
NomeRazao=CLIENTE TESTE LTDA
CNPJ=00003516000190
indIEDest=9
CEP=06730000
Logradouro=RUA TESTE
Numero=100
Bairro=CENTRO
cMun=3550308
Cidade=SAO PAULO
UF=SP
"@
        $escaped = $ini -replace '"', '""'
        $cmd = 'NFE.CriarNFe("' + $escaped + '",1)'
        $resp = Send-ACBrCommand $cmd
        Write-Host "`nResponse:`n$resp" -ForegroundColor White
    }

    2 {
        Write-Host "NFe with one product + ICMS" -ForegroundColor Yellow
        $ini = @"
[Identificacao]
NaturezaOperacao=VENDA
Modelo=55
Serie=001
Codigo=12121212
Numero=999999
Emissao=25/12/2025
Saida=25/12/2025
Tipo=1
finNFe=1
FormaPag=0
idDest=1
indPres=1
indFinal=1
[Emitente]
CNPJ=09375013000110
IE=206257840114
Razao=STACCATO REVESTIMENTOS
Fantasia=STACCATO
Fone=1141919816
CEP=06455000
Logradouro=Alameda Araguaia
Numero=661
Bairro=Alphaville
cMun=3505708
Cidade=Barueri
UF=SP
[Destinatario]
NomeRazao=CLIENTE TESTE LTDA
CNPJ=00003516000190
indIEDest=9
CEP=06730000
Logradouro=RUA TESTE
Numero=100
Bairro=CENTRO
cMun=3550308
Cidade=SAO PAULO
UF=SP
[Produto001]
CFOP=5102
NCM=69072200
Codigo=TESTE001
Descricao=PRODUTO TESTE
Unidade=UN
Quantidade=1
ValorUnitario=100.00
ValorTotal=100.00
[ICMS001]
CST=00
Modalidade=0
ValorBase=100.00
Aliquota=18.00
Valor=18.00
[Total]
BaseICMS=100.00
ValorICMS=18.00
ValorProduto=100.00
ValorNota=100.00
[Transportador]
FretePorConta=9
[Pag001]
tPag=90
vPag=100.00
"@
        $escaped = $ini -replace '"', '""'
        $cmd = 'NFE.CriarNFe("' + $escaped + '",1)'
        $resp = Send-ACBrCommand $cmd
        Write-Host "`nResponse:`n$resp" -ForegroundColor White
    }

    3 {
        Write-Host "NFe with ICMS + PIS + COFINS (Complete)" -ForegroundColor Yellow
        $ini = @"
[Identificacao]
NaturezaOperacao=VENDA
Modelo=55
Serie=001
Codigo=12121212
Numero=999999
Emissao=25/12/2025
Saida=25/12/2025
Tipo=1
finNFe=1
FormaPag=0
idDest=1
indPres=1
indFinal=1
[Emitente]
CNPJ=09375013000110
IE=206257840114
Razao=STACCATO REVESTIMENTOS
Fantasia=STACCATO
Fone=1141919816
CEP=06455000
Logradouro=Alameda Araguaia
Numero=661
Bairro=Alphaville
cMun=3505708
Cidade=Barueri
UF=SP
[Destinatario]
NomeRazao=CLIENTE TESTE LTDA
CNPJ=00003516000190
indIEDest=9
CEP=06730000
Logradouro=RUA TESTE
Numero=100
Bairro=CENTRO
cMun=3550308
Cidade=SAO PAULO
UF=SP
[Produto001]
CFOP=5102
NCM=69072200
Codigo=TESTE001
Descricao=PRODUTO TESTE
Unidade=UN
Quantidade=1
ValorUnitario=100.00
ValorTotal=100.00
[ICMS001]
CST=00
Modalidade=0
ValorBase=100.00
Aliquota=18.00
Valor=18.00
[PIS001]
CST=01
ValorBase=100.00
Aliquota=1.65
Valor=1.65
[COFINS001]
CST=01
ValorBase=100.00
Aliquota=7.60
Valor=7.60
[Total]
BaseICMS=100.00
ValorICMS=18.00
ValorPIS=1.65
ValorCOFINS=7.60
ValorProduto=100.00
ValorNota=100.00
[Transportador]
FretePorConta=9
[Pag001]
tPag=90
vPag=100.00
"@
        $escaped = $ini -replace '"', '""'
        $cmd = 'NFE.CriarNFe("' + $escaped + '",1)'
        $resp = Send-ACBrCommand $cmd
        Write-Host "`nResponse:`n$resp" -ForegroundColor White
    }

    4 {
        Write-Host "NFe with IBS/CBS (Tax Reform 2025)" -ForegroundColor Yellow
        $ini = @"
[Identificacao]
NaturezaOperacao=VENDA
Modelo=55
Serie=001
Codigo=12121212
Numero=999999
Emissao=25/12/2025
Saida=25/12/2025
Tipo=1
finNFe=1
FormaPag=0
idDest=1
indPres=1
indFinal=1
[Emitente]
CNPJ=09375013000110
IE=206257840114
Razao=STACCATO REVESTIMENTOS
Fantasia=STACCATO
Fone=1141919816
CEP=06455000
Logradouro=Alameda Araguaia
Numero=661
Bairro=Alphaville
cMun=3505708
Cidade=Barueri
UF=SP
[Destinatario]
NomeRazao=CLIENTE TESTE LTDA
CNPJ=00003516000190
indIEDest=9
CEP=06730000
Logradouro=RUA TESTE
Numero=100
Bairro=CENTRO
cMun=3550308
Cidade=SAO PAULO
UF=SP
[Produto001]
CFOP=5102
NCM=69072200
Codigo=TESTE001
Descricao=PRODUTO TESTE
Unidade=UN
Quantidade=1
ValorUnitario=100.00
ValorTotal=100.00
[ICMS001]
CST=00
Modalidade=0
ValorBase=100.00
Aliquota=18.00
Valor=18.00
[PIS001]
CST=01
ValorBase=100.00
Aliquota=1.65
Valor=1.65
[COFINS001]
CST=01
ValorBase=100.00
Aliquota=7.60
Valor=7.60
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
ValorIBSUF=0.10
ValorIBSMun=0.00
ValorCBS=0.90
ValorProduto=100.00
ValorNota=100.00
[Transportador]
FretePorConta=9
[Pag001]
tPag=90
vPag=100.00
"@
        $escaped = $ini -replace '"', '""'
        $cmd = 'NFE.CriarNFe("' + $escaped + '",1)'
        $resp = Send-ACBrCommand $cmd
        Write-Host "`nResponse:`n$resp" -ForegroundColor White
    }

    5 {
        Write-Host "Full NFe from app debug file" -ForegroundColor Yellow
        $debugFile = "C:\Builds\build-Loja-Desktop_Qt_5_15_2_MSVC2019_32bit-Release\release\nfe_acbr_command.txt"
        if (Test-Path $debugFile) {
            $cmd = Get-Content $debugFile -Raw -Encoding UTF8
            Write-Host "File size: $($cmd.Length) chars" -ForegroundColor Gray
            $resp = Send-ACBrCommand $cmd
            Write-Host "`nResponse:`n$resp" -ForegroundColor White
        } else {
            Write-Host "Debug file not found: $debugFile" -ForegroundColor Red
            Write-Host "Run the ERP first to generate the file." -ForegroundColor Yellow
        }
    }

    6 {
        Write-Host "Create + Validate NFe (with IBS/CBS)" -ForegroundColor Yellow
        $ini = @"
[Identificacao]
NaturezaOperacao=VENDA
Modelo=55
Serie=001
Codigo=12121212
Numero=999999
Emissao=25/12/2025
Saida=25/12/2025
Tipo=1
finNFe=1
FormaPag=0
idDest=1
indPres=1
indFinal=1
[Emitente]
CNPJ=09375013000110
IE=206257840114
Razao=STACCATO REVESTIMENTOS
Fantasia=STACCATO
Fone=1141919816
CEP=06455000
Logradouro=Alameda Araguaia
Numero=661
Bairro=Alphaville
cMun=3505708
Cidade=Barueri
UF=SP
[Destinatario]
NomeRazao=CLIENTE TESTE LTDA
CNPJ=00003516000190
indIEDest=9
CEP=06730000
Logradouro=RUA TESTE
Numero=100
Bairro=CENTRO
cMun=3550308
Cidade=SAO PAULO
UF=SP
[Produto001]
CFOP=5102
NCM=69072200
Codigo=TESTE001
Descricao=PRODUTO TESTE
Unidade=UN
Quantidade=1
ValorUnitario=100.00
ValorTotal=100.00
[ICMS001]
CST=00
Modalidade=0
ValorBase=100.00
Aliquota=18.00
Valor=18.00
[PIS001]
CST=01
ValorBase=100.00
Aliquota=1.65
Valor=1.65
[COFINS001]
CST=01
ValorBase=100.00
Aliquota=7.60
Valor=7.60
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
[Transportador]
FretePorConta=9
[Pag001]
tPag=90
vPag=100.00
"@
        # Step 1: Create NFe
        Write-Host "`n[Step 1] Creating NFe..." -ForegroundColor Cyan
        $escaped = $ini -replace '"', '""'
        $cmd = 'NFE.CriarNFe("' + $escaped + '",1)'
        $resp = Send-ACBrCommand $cmd

        if ($resp -match "^OK:") {
            # Extract XML path from response
            $lines = $resp -split "`n"
            $xmlPath = ($lines[0] -replace "^OK:\s*", "").Trim()
            Write-Host "NFe created: $xmlPath" -ForegroundColor Green

            # Step 2: Validate NFe
            Write-Host "`n[Step 2] Validating NFe..." -ForegroundColor Cyan
            $validateResp = Send-ACBrCommand "NFE.ValidarNFe(`"$xmlPath`")"
            Write-Host "Validation result:`n$validateResp" -ForegroundColor White
        } else {
            Write-Host "Failed to create NFe:`n$resp" -ForegroundColor Red
        }
    }

    7 {
        Write-Host "Create + Validate + Send to SEFAZ (HOMOLOGATION)" -ForegroundColor Yellow
        Write-Host "WARNING: This will send to SEFAZ HOMOLOGATION environment!" -ForegroundColor Red

        # Sequential numbering with counter file
        $counterFile = "$PSScriptRoot\nfe-counter.txt"
        if (Test-Path $counterFile) {
            $nfeNum = [int](Get-Content $counterFile) + 1
        } else {
            $nfeNum = 1
        }
        $nfeNum | Out-File $counterFile -NoNewline
        Write-Host "NFe Numero=$nfeNum" -ForegroundColor Gray

        $ini = @"
[Identificacao]
NaturezaOperacao=VENDA
Modelo=55
Serie=001
Codigo=12121212
Numero=$nfeNum
Emissao=25/12/2025
Saida=25/12/2025
Tipo=1
finNFe=1
FormaPag=0
idDest=1
indPres=1
indFinal=1
[Emitente]
CNPJ=09375013000110
IE=206257840114
Razao=STACCATO REVESTIMENTOS
Fantasia=STACCATO
Fone=1141919816
CEP=06455000
Logradouro=Alameda Araguaia
Numero=661
Bairro=Alphaville
cMun=3505708
Cidade=Barueri
UF=SP
[Destinatario]
NomeRazao=NF-E EMITIDA EM AMBIENTE DE HOMOLOGACAO - SEM VALOR FISCAL
CPF=12345678909
indIEDest=9
CEP=06730000
Logradouro=RUA TESTE
Numero=100
Bairro=CENTRO
cMun=3550308
Cidade=SAO PAULO
UF=SP
[Produto001]
CFOP=5102
NCM=69072200
Codigo=TESTE001
Descricao=PRODUTO TESTE
Unidade=UN
Quantidade=1
ValorUnitario=100.00
ValorTotal=100.00
[ICMS001]
CST=00
Modalidade=0
ValorBase=100.00
Aliquota=18.00
Valor=18.00
[PIS001]
CST=01
ValorBase=100.00
Aliquota=1.65
Valor=1.65
[COFINS001]
CST=01
ValorBase=100.00
Aliquota=7.60
Valor=7.60
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
[Pag001]
tPag=90
vPag=0.00
"@
        # Step 1: Create NFe
        Write-Host "`n[Step 1] Creating NFe..." -ForegroundColor Cyan
        $escaped = $ini -replace '"', '""'
        $cmd = 'NFE.CriarNFe("' + $escaped + '",1)'
        $resp = Send-ACBrCommand $cmd

        if ($resp -match "^OK:") {
            $lines = $resp -split "`n"
            $xmlPath = ($lines[0] -replace "^OK:\s*", "").Trim()
            Write-Host "NFe created: $xmlPath" -ForegroundColor Green

            # Step 2: Validate NFe
            Write-Host "`n[Step 2] Validating NFe..." -ForegroundColor Cyan
            $validateResp = Send-ACBrCommand "NFE.ValidarNFe(`"$xmlPath`")"

            if ($validateResp -match "^OK") {
                Write-Host "Validation OK" -ForegroundColor Green

                # Step 3: Send to SEFAZ
                Write-Host "`n[Step 3] Sending to SEFAZ (Homologation)..." -ForegroundColor Cyan
                $lote = Get-Random -Minimum 100000 -Maximum 999999
                # NFE.EnviarNFe(cArqXML, nLote, bAssina, bImprime, cImpressora, bSincrono, bValidaXML)
                $sendResp = Send-ACBrCommand "NFE.EnviarNFe(`"$xmlPath`",$lote,1,0,`"`",1,0)" -TimeoutSec 60
                Write-Host "SEFAZ Response:`n$sendResp" -ForegroundColor White
            } else {
                Write-Host "Validation failed:`n$validateResp" -ForegroundColor Red
            }
        } else {
            Write-Host "Failed to create NFe:`n$resp" -ForegroundColor Red
        }
    }

    8 {
        Write-Host "Test NomeArq behavior - verify authorized NFe path in response" -ForegroundColor Yellow
        Write-Host "This test sends an NFe and checks if ACBr returns NomeArq with the authorized file path" -ForegroundColor Gray

        $ini = @"
[Identificacao]
NaturezaOperacao=VENDA
Modelo=55
Serie=001
Codigo=12121212
Numero=999998
Emissao=25/12/2025
Saida=25/12/2025
Tipo=1
finNFe=1
FormaPag=0
idDest=1
indPres=1
indFinal=1
[Emitente]
CNPJ=09375013000110
IE=206257840114
Razao=STACCATO REVESTIMENTOS
Fantasia=STACCATO
Fone=1141919816
CEP=06455000
Logradouro=Alameda Araguaia
Numero=661
Bairro=Alphaville
cMun=3505708
Cidade=Barueri
UF=SP
[Destinatario]
NomeRazao=NF-E EMITIDA EM AMBIENTE DE HOMOLOGACAO - SEM VALOR FISCAL
CPF=12345678909
indIEDest=9
CEP=06730000
Logradouro=RUA TESTE
Numero=100
Bairro=CENTRO
cMun=3550308
Cidade=SAO PAULO
UF=SP
[Produto001]
CFOP=5403
NCM=69072200
Codigo=TESTE001
Descricao=PRODUTO TESTE
Unidade=UN
Quantidade=1
ValorUnitario=100.00
ValorTotal=100.00
[ICMS001]
CST=60
Modalidade=0
ValorBase=0.00
Aliquota=0.00
Valor=0.00
[PIS001]
CST=01
ValorBase=100.00
Aliquota=1.65
Valor=1.65
[COFINS001]
CST=01
ValorBase=100.00
Aliquota=7.60
Valor=7.60
[Total]
BaseICMS=0.00
ValorICMS=0.00
ValorPIS=1.65
ValorCOFINS=7.60
ValorProduto=100.00
ValorNota=100.00
[Transportador]
FretePorConta=9
[Pag001]
tPag=90
vPag=0.00
"@
        # Step 1: Create NFe
        Write-Host "`n[Step 1] Creating NFe..." -ForegroundColor Cyan
        $escaped = $ini -replace '"', '""'
        $cmd = 'NFE.CriarNFe("' + $escaped + '",1)'
        $resp = Send-ACBrCommand $cmd

        if ($resp -match "^OK:") {
            $lines = $resp -split "`n"
            $xmlPath = ($lines[0] -replace "^OK:\s*", "").Trim()
            Write-Host "NFe created: $xmlPath" -ForegroundColor Green

            # Step 2: Send to SEFAZ
            Write-Host "`n[Step 2] Sending to SEFAZ (Homologation)..." -ForegroundColor Cyan
            $lote = Get-Random -Minimum 100000 -Maximum 999999
            $sendResp = Send-ACBrCommand "NFE.EnviarNFe(`"$xmlPath`",$lote,1,0,`"`",1,0)" -TimeoutSec 60

            Write-Host "`n[Step 3] Analyzing response for NomeArq..." -ForegroundColor Cyan

            # Check for NomeArq in response
            if ($sendResp -match "NomeArq=([^\r\n]+)") {
                $nomeArq = $matches[1]
                Write-Host "SUCCESS: NomeArq found in response!" -ForegroundColor Green
                Write-Host "  Original path: $xmlPath" -ForegroundColor Gray
                Write-Host "  NomeArq path:  $nomeArq" -ForegroundColor Green

                if ($nomeArq -ne $xmlPath) {
                    Write-Host "`n  IMPORTANT: Paths are DIFFERENT!" -ForegroundColor Yellow
                    Write-Host "  ACBr saves authorized NFe to a different location." -ForegroundColor Yellow
                    Write-Host "  The ERP must use NomeArq path to load the authorized XML." -ForegroundColor Yellow
                } else {
                    Write-Host "`n  Paths are the same (ACBr updated file in place)" -ForegroundColor Gray
                }

                # Check if file exists
                if (Test-Path $nomeArq) {
                    Write-Host "`n  File exists at NomeArq path" -ForegroundColor Green
                    $content = Get-Content $nomeArq -Raw
                    if ($content -match "<dhRecbto>") {
                        Write-Host "  File contains <dhRecbto> (authorized)" -ForegroundColor Green
                    } else {
                        Write-Host "  WARNING: File does NOT contain <dhRecbto>" -ForegroundColor Red
                    }
                    if ($content -match "<nProt>") {
                        Write-Host "  File contains <nProt> (protocol number)" -ForegroundColor Green
                    } else {
                        Write-Host "  WARNING: File does NOT contain <nProt>" -ForegroundColor Red
                    }
                } else {
                    Write-Host "`n  WARNING: File does not exist at NomeArq path" -ForegroundColor Red
                }
            } else {
                Write-Host "WARNING: NomeArq NOT found in response!" -ForegroundColor Red
                Write-Host "This may indicate old ACBr behavior (file updated in place)" -ForegroundColor Yellow
            }

            Write-Host "`n[Full SEFAZ Response]" -ForegroundColor Cyan
            Write-Host $sendResp -ForegroundColor White
        } else {
            Write-Host "Failed to create NFe:`n$resp" -ForegroundColor Red
        }
    }

    default {
        Write-Host "Unknown stage: $Stage" -ForegroundColor Red
        Write-Host "Valid stages: 0-8" -ForegroundColor Yellow
    }
}

Write-Host "`n=== Test Complete ===" -ForegroundColor Cyan
