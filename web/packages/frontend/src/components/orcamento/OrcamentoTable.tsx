import { useRef, useCallback, useMemo } from 'react';
import { useVirtualizer } from '@tanstack/react-virtual';

interface OrcamentoTableProps {
  orcamentos: any[];
  onRowClick: (orcamento: any) => void;
  selectedOrcamento?: any | null;
}

const statusBadgeColors: Record<string, string> = {
  ATIVO: 'bg-green-900 text-green-300',
  EXPIRADO: 'bg-yellow-900 text-yellow-300',
  FECHADO: 'bg-blue-900 text-blue-300',
  PERDIDO: 'bg-red-900 text-red-300',
  CANCELADO: 'bg-slate-700 text-slate-300',
  REPLICADO: 'bg-purple-900 text-purple-300',
};

// Semaforo (followup) color mapping
const semaforoColors: Record<number, string> = {
  1: 'bg-red-900 text-red-300',      // QUENTE (Hot) - Red
  2: 'bg-amber-900 text-amber-300',  // MORNO (Warm) - Orange
  3: 'bg-blue-900 text-blue-300',    // FRIO (Cold) - Blue
};

function getRowBackgroundColor(orcamento: any): string {
  // FECHADO takes priority - special styling
  if (orcamento.status === 'FECHADO') {
    return 'bg-blue-950';
  }

  // CANCELADO and PERDIDO - yellow tint
  if (orcamento.status === 'CANCELADO' || orcamento.status === 'PERDIDO') {
    return 'bg-yellow-950';
  }

  // Color by semaforo (followup temperature) if available
  if (orcamento.semaforo && semaforoColors[orcamento.semaforo]) {
    return semaforoColors[orcamento.semaforo];
  }

  // Color by days remaining
  const dias = parseInt(String(orcamento.diasRestantes), 10);
  if (!isNaN(dias)) {
    if (dias < 3) return 'bg-red-950';      // < 3 days - Red
    if (dias < 5) return 'bg-yellow-950';   // 3-5 days - Yellow
    if (dias >= 5) return 'bg-green-950';   // >= 5 days - Green
  }

  // Default
  return 'hover:bg-slate-700';
}

// Memoize the currency formatter
const currencyFormatter = new Intl.NumberFormat('pt-BR', {
  style: 'currency',
  currency: 'BRL',
});

const dateFormatter = new Intl.DateTimeFormat('pt-BR');

function OrcamentoRow({ orcamento, isSelected, onRowClick }: any) {
  const formatDate = useCallback((date: any) => {
    if (!date) return '-';
    const d = new Date(date);
    return d.toLocaleDateString('pt-BR');
  }, []);

  const bgColor = isSelected ? 'bg-blue-800 ring-2 ring-blue-500' : getRowBackgroundColor(orcamento);

  return (
    <tr
      onClick={() => onRowClick(orcamento)}
      className={`cursor-pointer transition-colors ${bgColor}`}
    >
      <td className="px-6 py-3 whitespace-nowrap">
        <span
          className={`inline-flex items-center px-3 py-1 rounded-full text-xs font-medium ${
            statusBadgeColors[orcamento.status] || 'bg-slate-700 text-slate-300'
          }`}
        >
          {orcamento.status}
        </span>
      </td>
      <td className="px-6 py-3 whitespace-nowrap text-sm text-slate-400">
        {orcamento.diasRestantes || '-'}
      </td>
      <td className="px-6 py-3 whitespace-nowrap text-sm font-medium text-blue-400">
        {orcamento.idOrcamento}
      </td>
      <td className="px-6 py-3 whitespace-nowrap text-sm text-slate-400">
        {orcamento.vendedor || '-'}
      </td>
      <td className="px-6 py-3 whitespace-nowrap text-sm text-slate-400">
        {orcamento.consultor || '-'}
      </td>
      <td className="px-6 py-3 whitespace-nowrap text-sm text-slate-400">
        {orcamento.cliente || '-'}
      </td>
      <td className="px-6 py-3 whitespace-nowrap text-sm text-slate-400">
        {orcamento.profissional || '-'}
      </td>
      <td className="px-6 py-3 whitespace-nowrap text-xs text-slate-400">
        <div>
          {orcamento.tel && <div>{orcamento.tel}</div>}
          {orcamento.telCel && <div>{orcamento.telCel}</div>}
          {orcamento.telProf && <div>{orcamento.telProf}</div>}
        </div>
      </td>
      <td className="px-6 py-3 whitespace-nowrap text-sm text-slate-400">
        {formatDate(orcamento.data)}
      </td>
      <td className="px-6 py-3 whitespace-nowrap text-sm font-medium text-slate-300 text-right">
        {currencyFormatter.format(Number(orcamento.total) || 0)}
      </td>
      <td className="px-6 py-3 whitespace-nowrap text-sm text-slate-400">
        {formatDate(orcamento.dataFollowup)}
      </td>
      <td className="px-6 py-3 whitespace-nowrap text-sm text-slate-400">
        {formatDate(orcamento.dataProxFollowup)}
      </td>
      <td className="px-6 py-3 text-sm text-slate-400 max-w-xs truncate">
        {orcamento.observacao || '-'}
      </td>
    </tr>
  );
}

export function OrcamentoTable({ orcamentos, onRowClick, selectedOrcamento }: OrcamentoTableProps) {
  const parentRef = useRef<HTMLDivElement>(null);

  const rowVirtualizer = useVirtualizer({
    count: orcamentos.length,
    getScrollElement: () => parentRef.current,
    estimateSize: useCallback(() => 48, []), // ~48px per row (py-3 padding)
    overscan: 10, // Render 10 extra rows above/below viewport for smoother scrolling
  });

  const virtualRows = rowVirtualizer.getVirtualItems();
  const totalSize = rowVirtualizer.getTotalSize();

  const paddingTop = virtualRows.length > 0 ? virtualRows?.[0]?.start || 0 : 0;
  const paddingBottom = totalSize - (virtualRows[virtualRows.length - 1]?.end || 0);

  if (orcamentos.length === 0) {
    return (
      <div className="bg-slate-800 rounded-lg shadow-md p-8 text-center border border-slate-700">
        <p className="text-slate-400">Nenhum orçamento encontrado</p>
      </div>
    );
  }

  return (
    <div className="bg-slate-800 rounded-lg shadow-md overflow-hidden border border-slate-700 flex flex-col h-full">
      <div ref={parentRef} className="overflow-auto flex-1">
        <table className="w-full">
          <thead className="bg-slate-900 border-b border-slate-700 sticky top-0 z-10">
            <tr>
              <th className="px-6 py-3 text-left text-xs font-medium text-slate-300 uppercase tracking-wider">
                Status
              </th>
              <th className="px-6 py-3 text-left text-xs font-medium text-slate-300 uppercase tracking-wider">
                Dias
              </th>
              <th className="px-6 py-3 text-left text-xs font-medium text-slate-300 uppercase tracking-wider">
                Código
              </th>
              <th className="px-6 py-3 text-left text-xs font-medium text-slate-300 uppercase tracking-wider">
                Vendedor
              </th>
              <th className="px-6 py-3 text-left text-xs font-medium text-slate-300 uppercase tracking-wider">
                Consultor
              </th>
              <th className="px-6 py-3 text-left text-xs font-medium text-slate-300 uppercase tracking-wider">
                Cliente
              </th>
              <th className="px-6 py-3 text-left text-xs font-medium text-slate-300 uppercase tracking-wider">
                Profissional
              </th>
              <th className="px-6 py-3 text-left text-xs font-medium text-slate-300 uppercase tracking-wider">
                Telefones
              </th>
              <th className="px-6 py-3 text-left text-xs font-medium text-slate-300 uppercase tracking-wider">
                Data
              </th>
              <th className="px-6 py-3 text-right text-xs font-medium text-slate-300 uppercase tracking-wider">
                Total
              </th>
              <th className="px-6 py-3 text-left text-xs font-medium text-slate-300 uppercase tracking-wider">
                Últ. Followup
              </th>
              <th className="px-6 py-3 text-left text-xs font-medium text-slate-300 uppercase tracking-wider">
                Próx. Followup
              </th>
              <th className="px-6 py-3 text-left text-xs font-medium text-slate-300 uppercase tracking-wider">
                Observação
              </th>
            </tr>
          </thead>
          <tbody className="divide-y divide-slate-700">
            {paddingTop > 0 && (
              <tr>
                <td colSpan={13} style={{ height: `${paddingTop}px` }} />
              </tr>
            )}
            {virtualRows.map((virtualRow) => (
              <OrcamentoRow
                key={orcamentos[virtualRow.index].idOrcamento}
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

      {/* Info Footer */}
      <div className="bg-slate-900 border-t border-slate-700 px-6 py-2">
        <div className="text-xs text-slate-400">
          Total: {orcamentos.length} orçamentos | Mostrando {virtualRows.length} visíveis
        </div>
      </div>
    </div>
  );
}
