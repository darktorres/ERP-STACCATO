-- One-time repair + audit for QUEBRADO/reposição under-billing.
--
-- IMPORTANT: a zero-value 'REPO. ENTREGA' row is NOT a bug in general. It is the normal,
-- correct pattern when the broken boxes were already billed on the ORIGINAL NF-e: in
-- dividirVenda the QUEBRADO row inherits idNFeSaida from the original line, so the replacement
-- is a free remessa (total=0). Moving value onto those would DOUBLE-BILL the client.
--
-- The only real bug is the "break-first, then invoice" flow: the original line was never
-- invoiced (so its QUEBRADO partner is also un-invoiced), and the replacement is later invoiced
-- at zero -> under-billing. We repair only the not-yet-invoiced subset of those; the
-- already-emitted ones can't be changed retroactively and are exported for the accountant.
--
-- The repair requires `q.total > 0` (the QUEBRADO line actually carries the value we move).
-- Pairs where BOTH sides are zero are NOT fixable here: a QUEBRADO with total=0 has no value to
-- move (and for the few with prcUnitario>0 but quant/total=0, copying its fields would leave the
-- replacement with a unit price and a zero total -- incoherent). Those are split off by STEP 1b
-- for manual review (were the broken boxes supposed to be billed at all?).
--
-- Join key: REPO.idRelacionado = QUEBRADO.idVendaProduto2  (set in criarReposicaoCliente / dividirVenda).
-- Durable marker is the `reposicaoEntrega=1` flag, NOT status='REPO. ENTREGA' (status advances
-- to ENTREGUE etc. as the replacement progresses).
--
-- Run on the LIVE DB. Port 3306 is firewalled; connect via SSH to the server and run mysql
-- there (e.g. plink -ssh root@nuvem.staccatorevestimentos.com.br "... mysql -u root staccato").
-- Re-run STEP 1 first and review the exact rows before applying STEP 2.

USE `staccato`;

-- -----------------------------------------------------------------------------
-- STEP 1 -- DRY RUN: the safely-fixable rows (break-first, nothing invoiced yet).
--   repo.total=0, caixas>0, repo NOT invoiced, QUEBRADO partner NOT invoiced AND q.total>0,
--   excluding terminal statuses that will never be invoiced.
-- -----------------------------------------------------------------------------
SELECT repo.idVenda,
       repo.idVendaProduto2 AS repoId,
       q.idVendaProduto2    AS quebradoId,
       repo.status          AS repoStatus,
       repo.produto,
       repo.caixas          AS repoCaixas,
       q.total              AS quebradoTotal,   -- value that will move onto repo
       repo.total           AS repoTotal        -- currently 0
FROM venda_has_produto2 repo
JOIN venda_has_produto2 q ON repo.idRelacionado = q.idVendaProduto2
WHERE repo.reposicaoEntrega = 1 AND repo.total = 0 AND repo.caixas > 0
  AND repo.idNFeSaida IS NULL
  AND repo.status NOT IN ('CANCELADO','DEVOLVIDO','DEVOLVIDO ESTOQUE','DEVOLVIDO FORN.','PENDENTE DEV.')
  AND q.status = 'QUEBRADO' AND q.idNFeSaida IS NULL AND q.total > 0;

-- -----------------------------------------------------------------------------
-- STEP 1b -- REVIEW (read-only): the EXCLUDED no-value pairs (q.total = 0).
--   Same break-first shape as STEP 1 but the QUEBRADO line carries no value, so there is
--   nothing to move. Not fixable here -- inspect whether these boxes should have been billed.
-- -----------------------------------------------------------------------------
SELECT repo.idVenda,
       repo.idVendaProduto2 AS repoId,
       q.idVendaProduto2    AS quebradoId,
       repo.status          AS repoStatus,
       repo.produto,
       repo.caixas          AS repoCaixas,
       q.prcUnitario        AS quebradoPrcUnit,
       q.quant              AS quebradoQuant,
       q.total              AS quebradoTotal    -- 0
FROM venda_has_produto2 repo
JOIN venda_has_produto2 q ON repo.idRelacionado = q.idVendaProduto2
WHERE repo.reposicaoEntrega = 1 AND repo.total = 0 AND repo.caixas > 0
  AND repo.idNFeSaida IS NULL
  AND repo.status NOT IN ('CANCELADO','DEVOLVIDO','DEVOLVIDO ESTOQUE','DEVOLVIDO FORN.','PENDENTE DEV.')
  AND q.status = 'QUEBRADO' AND q.idNFeSaida IS NULL AND q.total = 0
ORDER BY repo.idVenda;

-- -----------------------------------------------------------------------------
-- STEP 2 -- APPLY: move price from QUEBRADO -> REPO. ENTREGA, then zero QUEBRADO.
-- Inside a transaction; review the row counts, then COMMIT manually.
-- The temp table snapshots the affected ids so the copy (reads QUEBRADO) and the zeroing
-- (writes QUEBRADO) never touch the same table in a single statement.
-- -----------------------------------------------------------------------------
START TRANSACTION;

CREATE TEMPORARY TABLE tmp_repo_fix AS
SELECT repo.idVendaProduto2 AS repoId, q.idVendaProduto2 AS quebradoId
FROM venda_has_produto2 repo
JOIN venda_has_produto2 q ON repo.idRelacionado = q.idVendaProduto2
WHERE repo.reposicaoEntrega = 1 AND repo.total = 0 AND repo.caixas > 0
  AND repo.idNFeSaida IS NULL
  AND repo.status NOT IN ('CANCELADO','DEVOLVIDO','DEVOLVIDO ESTOQUE','DEVOLVIDO FORN.','PENDENTE DEV.')
  AND q.status = 'QUEBRADO' AND q.idNFeSaida IS NULL AND q.total > 0;

UPDATE venda_has_produto2 repo
JOIN tmp_repo_fix t ON repo.idVendaProduto2 = t.repoId
JOIN venda_has_produto2 q ON q.idVendaProduto2 = t.quebradoId
SET repo.prcUnitario = q.prcUnitario,
    repo.descUnitario = q.descUnitario,
    repo.descGlobal   = q.descGlobal,
    repo.desconto     = q.desconto,
    repo.parcial      = q.parcial,
    repo.parcialDesc  = q.parcialDesc,
    repo.total        = q.total;

UPDATE venda_has_produto2 q
JOIN tmp_repo_fix t ON q.idVendaProduto2 = t.quebradoId
SET q.prcUnitario = 0,
    q.descUnitario = 0,
    q.descGlobal = 0,
    q.desconto = 0,
    q.parcial = 0,
    q.parcialDesc = 0,
    q.total = 0;

DROP TEMPORARY TABLE tmp_repo_fix;

-- Review the two UPDATE row counts (must match the STEP 1 count), then COMMIT manually.
-- ROLLBACK if anything looks off.
-- COMMIT;

-- -----------------------------------------------------------------------------
-- STEP 3 -- ACCOUNTANT EXPORT (read-only, no writes): already-emitted under-billing.
-- The replacement's zero-value NF-e was ALREADY issued (repo.idNFeSaida set) while the broken
-- boxes were never billed (QUEBRADO un-invoiced). These cannot be fixed in the DB -- they need
-- a fiscal correction (CC-e / NF-e complementar). repoNFe is the zero-value NF-e issued.
-- -----------------------------------------------------------------------------
SELECT repo.idVenda,
       repo.idVendaProduto2 AS repoId,
       q.idVendaProduto2    AS quebradoId,
       repo.status          AS repoStatus,
       repo.produto,
       repo.caixas          AS repoCaixas,
       q.total              AS valorNaoFaturado,
       repo.idNFeSaida      AS repoNFe
FROM venda_has_produto2 repo
JOIN venda_has_produto2 q ON repo.idRelacionado = q.idVendaProduto2
WHERE repo.reposicaoEntrega = 1 AND repo.total = 0 AND repo.caixas > 0
  AND repo.idNFeSaida IS NOT NULL
  AND q.status = 'QUEBRADO' AND q.idNFeSaida IS NULL AND q.total > 0
ORDER BY repo.idVenda;
