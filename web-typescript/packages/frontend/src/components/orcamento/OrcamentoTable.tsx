import { useRef, useCallback, useEffect } from 'react';
import { useVirtualizer } from '@tanstack/react-virtual';

interface OrcamentoTableProps {
  orcamentos: any[];
  onRowClick: (orcamento: any) => void;
  selectedOrcamento?: any | null;
}

const statusBadgeColors: Record<string, { bg: string; text: string }> = {
  ATIVO: { bg: '#1a3a2a', text: '#86efac' },
  EXPIRADO: { bg: '#3a2a1a', text: '#fcd34d' },
  FECHADO: { bg: '#1a2a3a', text: '#86c5fa' },
  PERDIDO: { bg: '#3a1a1a', text: '#f87171' },
  CANCELADO: { bg: '#2a2a3a', text: '#cbd5e1' },
  REPLICADO: { bg: '#2a1a3a', text: '#d8b4fe' },
};

const semaforoColors: Record<number, string> = {
  1: '#1a1a2a', // QUENTE
  2: '#2a1a0a', // MORNO
  3: '#0a1a2a', // FRIO
};

function getRowBackgroundColor(orcamento: any): string {
  if (orcamento.status === 'FECHADO') return '#001a33';
  if (orcamento.status === 'CANCELADO' || orcamento.status === 'PERDIDO') return '#1a1a00';
  if (orcamento.semaforo && semaforoColors[orcamento.semaforo]) return semaforoColors[orcamento.semaforo];

  const dias = parseInt(String(orcamento.diasRestantes), 10);
  if (!isNaN(dias)) {
    if (dias < 3) return '#1a0000';
    if (dias < 5) return '#1a1a00';
    if (dias >= 5) return '#001a00';
  }
  return 'transparent';
}

const currencyFormatter = new Intl.NumberFormat('pt-BR', {
  style: 'currency',
  currency: 'BRL',
});

function formatDate(date: any): string {
  if (!date) return '-';
  const d = new Date(date);
  return d.toLocaleDateString('pt-BR');
}

// Ultra-lightweight row component with inline styles
const OrcamentoRow = ({ orcamento, isSelected, onRowClick }: any) => {
  const bgColor = isSelected ? '#1e3a8a' : getRowBackgroundColor(orcamento);
  const statusColor = statusBadgeColors[orcamento.status] || { bg: '#2a2a3a', text: '#cbd5e1' };

  return (
    <tr
      onClick={() => onRowClick(orcamento)}
      style={{
        backgroundColor: bgColor,
        cursor: 'pointer',
        transition: 'background-color 0.2s',
      }}
    >
      <td style={{ padding: '12px 24px', whiteSpace: 'nowrap' }}>
        <span
          style={{
            display: 'inline-flex',
            alignItems: 'center',
            padding: '4px 12px',
            borderRadius: '9999px',
            fontSize: '12px',
            fontWeight: '500',
            backgroundColor: statusColor.bg,
            color: statusColor.text,
          }}
        >
          {orcamento.status}
        </span>
      </td>
      <td style={{ padding: '12px 24px', whiteSpace: 'nowrap', fontSize: '14px', color: '#94a3b8' }}>
        {orcamento.diasRestantes || '-'}
      </td>
      <td style={{ padding: '12px 24px', whiteSpace: 'nowrap', fontSize: '14px', fontWeight: '500', color: '#60a5fa' }}>
        {orcamento.idOrcamento}
      </td>
      <td style={{ padding: '12px 24px', whiteSpace: 'nowrap', fontSize: '14px', color: '#94a3b8' }}>
        {orcamento.vendedor || '-'}
      </td>
      <td style={{ padding: '12px 24px', whiteSpace: 'nowrap', fontSize: '14px', color: '#94a3b8' }}>
        {orcamento.consultor || '-'}
      </td>
      <td style={{ padding: '12px 24px', whiteSpace: 'nowrap', fontSize: '14px', color: '#94a3b8' }}>
        {orcamento.cliente || '-'}
      </td>
      <td style={{ padding: '12px 24px', whiteSpace: 'nowrap', fontSize: '14px', color: '#94a3b8' }}>
        {orcamento.profissional || '-'}
      </td>
      <td style={{ padding: '12px 24px', whiteSpace: 'nowrap', fontSize: '12px', color: '#94a3b8' }}>
        {orcamento.tel && <div>{orcamento.tel}</div>}
        {orcamento.telCel && <div>{orcamento.telCel}</div>}
        {orcamento.telProf && <div>{orcamento.telProf}</div>}
      </td>
      <td style={{ padding: '12px 24px', whiteSpace: 'nowrap', fontSize: '14px', color: '#94a3b8' }}>
        {formatDate(orcamento.data)}
      </td>
      <td style={{ padding: '12px 24px', whiteSpace: 'nowrap', fontSize: '14px', fontWeight: '500', color: '#cbd5e1', textAlign: 'right' }}>
        {currencyFormatter.format(Number(orcamento.total) || 0)}
      </td>
      <td style={{ padding: '12px 24px', whiteSpace: 'nowrap', fontSize: '14px', color: '#94a3b8' }}>
        {formatDate(orcamento.dataFollowup)}
      </td>
      <td style={{ padding: '12px 24px', whiteSpace: 'nowrap', fontSize: '14px', color: '#94a3b8' }}>
        {formatDate(orcamento.dataProxFollowup)}
      </td>
      <td style={{ padding: '12px 24px', fontSize: '14px', color: '#94a3b8', maxWidth: '320px', overflow: 'hidden', textOverflow: 'ellipsis' }}>
        {orcamento.observacao || '-'}
      </td>
    </tr>
  );
};

export function OrcamentoTable({ orcamentos, onRowClick, selectedOrcamento }: OrcamentoTableProps) {
  const parentRef = useRef<HTMLDivElement>(null);
  const renderStartRef = useRef(performance.now());

  const rowVirtualizer = useVirtualizer({
    count: orcamentos.length,
    getScrollElement: () => parentRef.current,
    estimateSize: useCallback(() => 48, []),
    overscan: 10,
  });

  const virtualRows = rowVirtualizer.getVirtualItems();
  const totalSize = rowVirtualizer.getTotalSize();

  // Log render timing
  useEffect(() => {
    if (virtualRows.length > 0) {
      const elapsed = performance.now() - renderStartRef.current;
      console.log(`[OrcamentoTable] Rendered in ${elapsed.toFixed(2)}ms, showing ${virtualRows.length} rows of ${orcamentos.length}`);
      renderStartRef.current = 0; // Only log once per data change
    }
  }, [virtualRows.length, orcamentos.length]);

  const paddingTop = virtualRows.length > 0 ? virtualRows[0]?.start || 0 : 0;
  const paddingBottom = totalSize - (virtualRows[virtualRows.length - 1]?.end || 0);

  if (orcamentos.length === 0) {
    return (
      <div style={{ backgroundColor: '#1e293b', borderRadius: '8px', padding: '32px', textAlign: 'center', border: '1px solid #334155' }}>
        <p style={{ color: '#64748b' }}>Nenhum orçamento encontrado</p>
      </div>
    );
  }

  return (
    <div style={{ backgroundColor: '#1e293b', borderRadius: '8px', overflow: 'hidden', border: '1px solid #334155', display: 'flex', flexDirection: 'column', height: '100%' }}>
      <div ref={parentRef} style={{ overflowY: 'auto', flex: 1 }}>
        <table style={{ width: '100%', borderCollapse: 'collapse' }}>
          <thead style={{ backgroundColor: '#0f172a', borderBottom: '1px solid #334155', position: 'sticky', top: 0, zIndex: 10 }}>
            <tr>
              <th style={{ padding: '12px 24px', textAlign: 'left', fontSize: '12px', fontWeight: '500', color: '#cbd5e1', textTransform: 'uppercase', letterSpacing: '0.05em' }}>Status</th>
              <th style={{ padding: '12px 24px', textAlign: 'left', fontSize: '12px', fontWeight: '500', color: '#cbd5e1', textTransform: 'uppercase', letterSpacing: '0.05em' }}>Dias</th>
              <th style={{ padding: '12px 24px', textAlign: 'left', fontSize: '12px', fontWeight: '500', color: '#cbd5e1', textTransform: 'uppercase', letterSpacing: '0.05em' }}>Código</th>
              <th style={{ padding: '12px 24px', textAlign: 'left', fontSize: '12px', fontWeight: '500', color: '#cbd5e1', textTransform: 'uppercase', letterSpacing: '0.05em' }}>Vendedor</th>
              <th style={{ padding: '12px 24px', textAlign: 'left', fontSize: '12px', fontWeight: '500', color: '#cbd5e1', textTransform: 'uppercase', letterSpacing: '0.05em' }}>Consultor</th>
              <th style={{ padding: '12px 24px', textAlign: 'left', fontSize: '12px', fontWeight: '500', color: '#cbd5e1', textTransform: 'uppercase', letterSpacing: '0.05em' }}>Cliente</th>
              <th style={{ padding: '12px 24px', textAlign: 'left', fontSize: '12px', fontWeight: '500', color: '#cbd5e1', textTransform: 'uppercase', letterSpacing: '0.05em' }}>Profissional</th>
              <th style={{ padding: '12px 24px', textAlign: 'left', fontSize: '12px', fontWeight: '500', color: '#cbd5e1', textTransform: 'uppercase', letterSpacing: '0.05em' }}>Telefones</th>
              <th style={{ padding: '12px 24px', textAlign: 'left', fontSize: '12px', fontWeight: '500', color: '#cbd5e1', textTransform: 'uppercase', letterSpacing: '0.05em' }}>Data</th>
              <th style={{ padding: '12px 24px', textAlign: 'right', fontSize: '12px', fontWeight: '500', color: '#cbd5e1', textTransform: 'uppercase', letterSpacing: '0.05em' }}>Total</th>
              <th style={{ padding: '12px 24px', textAlign: 'left', fontSize: '12px', fontWeight: '500', color: '#cbd5e1', textTransform: 'uppercase', letterSpacing: '0.05em' }}>Últ. Followup</th>
              <th style={{ padding: '12px 24px', textAlign: 'left', fontSize: '12px', fontWeight: '500', color: '#cbd5e1', textTransform: 'uppercase', letterSpacing: '0.05em' }}>Próx. Followup</th>
              <th style={{ padding: '12px 24px', textAlign: 'left', fontSize: '12px', fontWeight: '500', color: '#cbd5e1', textTransform: 'uppercase', letterSpacing: '0.05em' }}>Observação</th>
            </tr>
          </thead>
          <tbody style={{ borderTop: '1px solid #334155' }}>
            {paddingTop > 0 && (
              <tr>
                <td colSpan={13} style={{ height: `${paddingTop}px` }} />
              </tr>
            )}
            {virtualRows.map((virtualRow) => (
              <OrcamentoRow
                key={`${orcamentos[virtualRow.index].idOrcamento}-${virtualRow.index}`}
                orcamento={orcamentos[virtualRow.index]}
                isSelected={selectedOrcamento?.idOrcamento === orcamentos[virtualRow.index].idOrcamento}
                onRowClick={onRowClick}
              />
            ))}
            {paddingBottom > 0 && (
              <tr>
                <td colSpan={13} style={{ height: `${paddingBottom}px` }} />
              </tr>
            )}
          </tbody>
        </table>
      </div>

      <div style={{ backgroundColor: '#0f172a', borderTop: '1px solid #334155', padding: '8px 24px' }}>
        <div style={{ fontSize: '12px', color: '#64748b' }}>
          Total: {orcamentos.length} orçamentos | Mostrando {virtualRows.length} visíveis
        </div>
      </div>
    </div>
  );
}
