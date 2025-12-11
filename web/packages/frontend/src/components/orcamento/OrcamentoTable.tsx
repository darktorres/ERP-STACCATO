interface OrcamentoTableProps {
  orcamentos: any[];
  onRowClick: (orcamento: any) => void;
}

const statusBadgeColors: Record<string, string> = {
  ATIVO: 'bg-green-100 text-green-800',
  EXPIRADO: 'bg-yellow-100 text-yellow-800',
  FECHADO: 'bg-blue-100 text-blue-800',
  PERDIDO: 'bg-red-100 text-red-800',
  CANCELADO: 'bg-gray-100 text-gray-800',
  REPLICADO: 'bg-purple-100 text-purple-800',
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
      <div className="bg-white rounded-lg shadow-md p-8 text-center">
        <p className="text-gray-500">Nenhum orçamento encontrado</p>
      </div>
    );
  }

  return (
    <div className="bg-white rounded-lg shadow-md overflow-hidden">
      <div className="overflow-x-auto">
        <table className="w-full">
          <thead className="bg-gray-100 border-b border-gray-200">
            <tr>
              <th className="px-6 py-3 text-left text-xs font-medium text-gray-700 uppercase tracking-wider">
                Status
              </th>
              <th className="px-6 py-3 text-left text-xs font-medium text-gray-700 uppercase tracking-wider">
                Dias
              </th>
              <th className="px-6 py-3 text-left text-xs font-medium text-gray-700 uppercase tracking-wider">
                Código
              </th>
              <th className="px-6 py-3 text-left text-xs font-medium text-gray-700 uppercase tracking-wider">
                Vendedor
              </th>
              <th className="px-6 py-3 text-left text-xs font-medium text-gray-700 uppercase tracking-wider">
                Consultor
              </th>
              <th className="px-6 py-3 text-left text-xs font-medium text-gray-700 uppercase tracking-wider">
                Cliente
              </th>
              <th className="px-6 py-3 text-left text-xs font-medium text-gray-700 uppercase tracking-wider">
                Profissional
              </th>
              <th className="px-6 py-3 text-left text-xs font-medium text-gray-700 uppercase tracking-wider">
                Telefones
              </th>
              <th className="px-6 py-3 text-left text-xs font-medium text-gray-700 uppercase tracking-wider">
                Data
              </th>
              <th className="px-6 py-3 text-right text-xs font-medium text-gray-700 uppercase tracking-wider">
                Total
              </th>
            </tr>
          </thead>
          <tbody className="divide-y divide-gray-200">
            {orcamentos.map((orcamento, idx) => (
              <tr
                key={idx}
                onClick={() => onRowClick(orcamento)}
                className="hover:bg-gray-50 cursor-pointer transition-colors"
              >
                <td className="px-6 py-4 whitespace-nowrap">
                  <span
                    className={`inline-flex items-center px-3 py-1 rounded-full text-xs font-medium ${
                      statusBadgeColors[orcamento.status] || 'bg-gray-100 text-gray-800'
                    }`}
                  >
                    {orcamento.status}
                  </span>
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-600">
                  {orcamento.diasRestantes || '-'}
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-sm font-medium text-blue-600">
                  {orcamento.idOrcamento}
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-600">
                  {orcamento.vendedor || '-'}
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-600">
                  {orcamento.consultor || '-'}
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-600">
                  {orcamento.cliente || '-'}
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-600">
                  {orcamento.profissional || '-'}
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-xs text-gray-600">
                  <div>
                    {orcamento.tel && <div>{orcamento.tel}</div>}
                    {orcamento.telCel && <div>{orcamento.telCel}</div>}
                    {orcamento.telProf && <div>{orcamento.telProf}</div>}
                  </div>
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-600">
                  {formatDate(orcamento.data)}
                </td>
                <td className="px-6 py-4 whitespace-nowrap text-sm font-medium text-gray-900 text-right">
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
