interface OrcamentoTableProps {
  orcamentos: any[];
  onRowClick: (orcamento: any) => void;
}

const statusBadgeColors: Record<string, string> = {
  ATIVO: 'bg-green-900 text-green-300',
  EXPIRADO: 'bg-yellow-900 text-yellow-300',
  FECHADO: 'bg-blue-900 text-blue-300',
  PERDIDO: 'bg-red-900 text-red-300',
  CANCELADO: 'bg-slate-700 text-slate-300',
  REPLICADO: 'bg-purple-900 text-purple-300',
};

export function OrcamentoTable({ orcamentos, onRowClick }: OrcamentoTableProps) {
  const formatCurrency = (value: number) => {
    return new Intl.NumberFormat('pt-BR', {
      style: 'currency',
      currency: 'BRL',
    }).format(value);
  };

  const formatDate = (date: any) => {
    if (!date) return '-';
    const d = new Date(date);
    return d.toLocaleDateString('pt-BR');
  };

  if (orcamentos.length === 0) {
    return (
      <div className="bg-slate-800 rounded-lg shadow-md p-8 text-center border border-slate-700">
        <p className="text-slate-400">Nenhum orçamento encontrado</p>
      </div>
    );
  }

  return (
    <div className="bg-slate-800 rounded-lg shadow-md overflow-hidden border border-slate-700">
      <div className="overflow-x-auto">
        <table className="w-full">
          <thead className="bg-slate-900 border-b border-slate-700">
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
            </tr>
          </thead>
          <tbody className="divide-y divide-slate-700">
            {orcamentos.map((orcamento, idx) => (
              <tr
                key={idx}
                onClick={() => onRowClick(orcamento)}
                className="hover:bg-slate-700 cursor-pointer transition-colors"
              >
                <td className="px-6 py-4 whitespace-nowrap">
                  <span
                    className={`inline-flex items-center px-3 py-1 rounded-full text-xs font-medium ${
                      statusBadgeColors[orcamento.status] || 'bg-slate-700 text-slate-300'
                    }`}
                  >
                    {orcamento.status}
                  </span>
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-sm text-slate-400">
                  {orcamento.diasRestantes || '-'}
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-sm font-medium text-blue-400">
                  {orcamento.idOrcamento}
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-sm text-slate-400">
                  {orcamento.vendedor || '-'}
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-sm text-slate-400">
                  {orcamento.consultor || '-'}
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-sm text-slate-400">
                  {orcamento.cliente || '-'}
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-sm text-slate-400">
                  {orcamento.profissional || '-'}
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-xs text-slate-400">
                  <div>
                    {orcamento.tel && <div>{orcamento.tel}</div>}
                    {orcamento.telCel && <div>{orcamento.telCel}</div>}
                    {orcamento.telProf && <div>{orcamento.telProf}</div>}
                  </div>
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-sm text-slate-400">
                  {formatDate(orcamento.data)}
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-sm font-medium text-slate-300 text-right">
                  {formatCurrency(orcamento.total)}
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  );
}
