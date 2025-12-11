import { OrcamentoFilters, OrcamentoStatusEnum } from '@erp-staccato/shared';

interface OrcamentoFilterPanelProps {
  filters: OrcamentoFilters;
  onFiltersChange: (filters: OrcamentoFilters) => void;
  lojas: Array<{ idLoja: number; descricao?: string; nomeFantasia: string }>;
  vendedores: Array<{ idUsuario: number; nome: string }>;
  fornecedores: Array<{ razaoSocial: string }>;
  userType?: string;
}

export function OrcamentoFilterPanel({
  filters,
  onFiltersChange,
  lojas,
  vendedores,
  fornecedores,
  userType,
}: OrcamentoFilterPanelProps) {
  const handleStatusChange = (status: string) => {
    const statuses = filters.statuses || [];
    const newStatuses = statuses.includes(status as any)
      ? statuses.filter((s) => s !== status)
      : [...statuses, status as any];
    onFiltersChange({ ...filters, statuses: newStatuses });
  };

  const handleLojaChange = (idLoja: number | undefined) => {
    onFiltersChange({ ...filters, idLoja });
  };

  const handleVendedorChange = (idVendedor: number | undefined) => {
    onFiltersChange({ ...filters, idVendedor });
  };

  const handleFornecedorChange = (fornecedor: string | undefined) => {
    onFiltersChange({ ...filters, fornecedor });
  };

  const handleSearchChange = (search: string) => {
    onFiltersChange({ ...filters, search: search || undefined });
  };

  const handleMesAnoChange = (mesAno: string) => {
    onFiltersChange({ ...filters, mesAno: mesAno || undefined });
  };

  const handlePropriosChange = (checked: boolean) => {
    onFiltersChange({ ...filters, apenasPropriosOrcamentos: checked });
  };

  return (
    <div className="bg-white rounded-lg shadow-md p-6 mb-6">
      <div className="space-y-6">
        {/* Status Checkboxes */}
        <div>
          <h3 className="text-sm font-semibold text-gray-900 mb-3">Status</h3>
          <div className="grid grid-cols-3 gap-4">
            {Object.entries(OrcamentoStatusEnum).map(([key, value]) => (
              <label key={key} className="flex items-center">
                <input
                  type="checkbox"
                  checked={(filters.statuses || []).includes(value as any)}
                  onChange={(e) => handleStatusChange(e.target.value)}
                  value={value}
                  className="h-4 w-4 text-blue-600 border-gray-300 rounded"
                />
                <span className="ml-2 text-sm text-gray-700">{value}</span>
              </label>
            ))}
          </div>
        </div>

        {/* Loja, Vendedor, Fornecedor Row */}
        <div className="grid grid-cols-3 gap-4">
          {/* Loja - Hidden for gerentes */}
          {userType !== 'GERENTE LOJA' && userType !== 'GERENTE DEPARTAMENTO' && (
            <div>
              <label className="block text-sm font-medium text-gray-700 mb-2">
                Loja
              </label>
              <select
                value={filters.idLoja || ''}
                onChange={(e) => handleLojaChange(e.target.value ? Number(e.target.value) : undefined)}
                className="w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-blue-500 focus:border-blue-500"
              >
                <option value="">Todas as lojas</option>
                {lojas.map((loja) => (
                  <option key={loja.idLoja} value={loja.idLoja}>
                    {loja.descricao || loja.nomeFantasia}
                  </option>
                ))}
              </select>
            </div>
          )}

          {/* Vendedor - Hidden for vendedores */}
          {userType !== 'VENDEDOR' && userType !== 'VENDEDOR ESPECIAL' && (
            <div>
              <label className="block text-sm font-medium text-gray-700 mb-2">
                Vendedor
              </label>
              <select
                value={filters.idVendedor || ''}
                onChange={(e) =>
                  handleVendedorChange(e.target.value ? Number(e.target.value) : undefined)
                }
                className="w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-blue-500 focus:border-blue-500"
              >
                <option value="">Todos os vendedores</option>
                {vendedores.map((vendedor) => (
                  <option key={vendedor.idUsuario} value={vendedor.idUsuario}>
                    {vendedor.nome}
                  </option>
                ))}
              </select>
            </div>
          )}

          {/* Fornecedor */}
          <div>
            <label className="block text-sm font-medium text-gray-700 mb-2">
              Fornecedor
            </label>
            <select
              value={filters.fornecedor || ''}
              onChange={(e) => handleFornecedorChange(e.target.value || undefined)}
              className="w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-blue-500 focus:border-blue-500"
            >
              <option value="">Todos os fornecedores</option>
              {fornecedores.map((fornecedor) => (
                <option key={fornecedor.razaoSocial} value={fornecedor.razaoSocial}>
                  {fornecedor.razaoSocial}
                </option>
              ))}
            </select>
          </div>
        </div>

        {/* Search and Month Row */}
        <div className="grid grid-cols-2 gap-4">
          {/* Search */}
          <div>
            <label className="block text-sm font-medium text-gray-700 mb-2">
              Buscar (Código, Cliente, Profissional)
            </label>
            <input
              type="text"
              placeholder="Digite para buscar..."
              value={filters.search || ''}
              onChange={(e) => handleSearchChange(e.target.value)}
              className="w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-blue-500 focus:border-blue-500"
            />
          </div>

          {/* Month */}
          {userType !== 'GERENTE LOJA' && userType !== 'GERENTE DEPARTAMENTO' && (
            <div>
              <label className="block text-sm font-medium text-gray-700 mb-2">
                Mês/Ano
              </label>
              <input
                type="month"
                value={filters.mesAno || ''}
                onChange={(e) => handleMesAnoChange(e.target.value)}
                className="w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-blue-500 focus:border-blue-500"
              />
            </div>
          )}
        </div>

        {/* Radio buttons - Todos vs Próprios */}
        {(userType === 'VENDEDOR' || userType === 'VENDEDOR ESPECIAL') && (
          <div>
            <h3 className="text-sm font-semibold text-gray-900 mb-3">Filtro de Orçamentos</h3>
            <div className="space-y-2">
              <label className="flex items-center">
                <input
                  type="radio"
                  checked={!filters.apenasPropriosOrcamentos}
                  onChange={(e) => handlePropriosChange(!e.target.checked)}
                  className="h-4 w-4 text-blue-600 border-gray-300"
                />
                <span className="ml-2 text-sm text-gray-700">Todos os orçamentos</span>
              </label>
              <label className="flex items-center">
                <input
                  type="radio"
                  checked={filters.apenasPropriosOrcamentos}
                  onChange={(e) => handlePropriosChange(e.target.checked)}
                  className="h-4 w-4 text-blue-600 border-gray-300"
                />
                <span className="ml-2 text-sm text-gray-700">Apenas meus orçamentos</span>
              </label>
            </div>
          </div>
        )}
      </div>
    </div>
  );
}
