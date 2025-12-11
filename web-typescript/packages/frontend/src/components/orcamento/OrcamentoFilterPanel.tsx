import { OrcamentoFilters, OrcamentoStatusEnum } from '@erp-staccato/shared';
import { useState, useRef, useEffect } from 'react';

interface OrcamentoFilterPanelProps {
  filters: OrcamentoFilters;
  onFiltersChange: (filters: OrcamentoFilters) => void;
  lojas: Array<{ idLoja: number; descricao?: string; nomeFantasia: string }>;
  vendedores: Array<{ idUsuario: number; nome: string }>;
  fornecedores: Array<{ razaoSocial: string }>;
  userType?: string;
}

const SEMAFORO_OPTIONS = [
  { value: undefined, label: 'Followup' },
  { value: 1, label: 'QUENTE' },
  { value: 2, label: 'MORNO' },
  { value: 3, label: 'FRIO' },
];

export function OrcamentoFilterPanel({
  filters,
  onFiltersChange,
  lojas,
  vendedores,
  fornecedores,
  userType,
}: OrcamentoFilterPanelProps) {
  const [statusEnabled, setStatusEnabled] = useState((filters.statuses || []).length > 0);
  const [searchText, setSearchText] = useState(filters.search || '');
  const searchTimeoutRef = useRef<NodeJS.Timeout | null>(null);

  // Debounce search
  useEffect(() => {
    if (searchTimeoutRef.current) {
      clearTimeout(searchTimeoutRef.current);
    }

    searchTimeoutRef.current = setTimeout(() => {
      onFiltersChange({ ...filters, search: searchText || undefined });
    }, 500);

    return () => {
      if (searchTimeoutRef.current) {
        clearTimeout(searchTimeoutRef.current);
      }
    };
  }, [searchText]);

  const handleStatusToggle = (enabled: boolean) => {
    setStatusEnabled(enabled);
    if (!enabled) {
      // When status filter is disabled, clear all status filters
      onFiltersChange({ ...filters, statuses: [] });
    }
  };

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
    setSearchText(search);
  };

  const handleMesAnoChange = (mesAno: string) => {
    onFiltersChange({ ...filters, mesAno: mesAno || undefined });
  };

  const handlePropriosChange = (checked: boolean) => {
    onFiltersChange({ ...filters, apenasPropriosOrcamentos: checked });
  };

  const handleSemaforoChange = (semaforo: number | undefined) => {
    onFiltersChange({ ...filters, semaforo });
  };

  return (
    <div className="bg-slate-800 rounded-lg shadow-md p-6 mb-6 border border-slate-700">
      <div className="space-y-6">
        {/* Status GroupBox */}
        <fieldset className="border border-slate-600 rounded-lg p-4">
          <legend className="px-2 text-sm font-semibold text-slate-50 flex items-center">
            <input
              type="checkbox"
              checked={statusEnabled}
              onChange={(e) => handleStatusToggle(e.target.checked)}
              className="h-4 w-4 text-blue-600 border-slate-600 rounded bg-slate-700 mr-2"
            />
            Status
          </legend>
          <div className="grid grid-cols-3 gap-4 mt-2">
            {Object.entries(OrcamentoStatusEnum).map(([key, value]) => (
              <label key={key} className="flex items-center">
                <input
                  type="checkbox"
                  checked={(filters.statuses || []).includes(value as any)}
                  onChange={(e) => handleStatusChange(e.target.value)}
                  value={value}
                  disabled={!statusEnabled}
                  className="h-4 w-4 text-blue-600 border-slate-600 rounded bg-slate-700 disabled:opacity-50 disabled:cursor-not-allowed"
                />
                <span className={`ml-2 text-sm ${statusEnabled ? 'text-slate-300' : 'text-slate-500'}`}>{value}</span>
              </label>
            ))}
          </div>
        </fieldset>

        {/* Loja, Vendedor, Fornecedor Row */}
        <div className="grid grid-cols-3 gap-4">
          {/* Loja - Hidden for gerentes */}
          {userType !== 'GERENTE LOJA' && userType !== 'GERENTE DEPARTAMENTO' && (
            <div>
              <label className="block text-sm font-medium text-slate-300 mb-2">
                Loja
              </label>
              <select
                value={filters.idLoja || ''}
                onChange={(e) => handleLojaChange(e.target.value ? Number(e.target.value) : undefined)}
                className="w-full px-3 py-2 border border-slate-600 rounded-md bg-slate-700 text-slate-50 shadow-sm focus:outline-none focus:ring-blue-400 focus:border-blue-400"
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
              <label className="block text-sm font-medium text-slate-300 mb-2">
                Vendedor
              </label>
              <select
                value={filters.idVendedor || ''}
                onChange={(e) =>
                  handleVendedorChange(e.target.value ? Number(e.target.value) : undefined)
                }
                className="w-full px-3 py-2 border border-slate-600 rounded-md bg-slate-700 text-slate-50 shadow-sm focus:outline-none focus:ring-blue-400 focus:border-blue-400"
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
            <label className="block text-sm font-medium text-slate-300 mb-2">
              Fornecedor
            </label>
            <select
              value={filters.fornecedor || ''}
              onChange={(e) => handleFornecedorChange(e.target.value || undefined)}
              className="w-full px-3 py-2 border border-slate-600 rounded-md bg-slate-700 text-slate-50 shadow-sm focus:outline-none focus:ring-blue-400 focus:border-blue-400"
            >
              <option value="">Todos os fornecedores</option>
              {fornecedores.map((fornecedor) => (
                <option key={fornecedor.razaoSocial} value={fornecedor.razaoSocial}>
                  {fornecedor.razaoSocial}
                </option>
              ))}
            </select>
          </div>

          {/* Followup Semaforo */}
          <div>
            <label className="block text-sm font-medium text-slate-300 mb-2">
              Seguimento
            </label>
            <select
              value={filters.semaforo?.toString() || ''}
              onChange={(e) => handleSemaforoChange(e.target.value ? Number(e.target.value) : undefined)}
              className="w-full px-3 py-2 border border-slate-600 rounded-md bg-slate-700 text-slate-50 shadow-sm focus:outline-none focus:ring-blue-400 focus:border-blue-400"
            >
              {SEMAFORO_OPTIONS.map((option) => (
                <option key={option.value ?? 'none'} value={option.value?.toString() || ''}>
                  {option.label}
                </option>
              ))}
            </select>
          </div>
        </div>

        {/* Search and Month Row */}
        <div className="grid grid-cols-2 gap-4">
          {/* Search */}
          <div>
            <label className="block text-sm font-medium text-slate-300 mb-2">
              Buscar (Código, Cliente, Profissional)
            </label>
            <input
              type="text"
              placeholder="Digite para buscar..."
              value={searchText}
              onChange={(e) => handleSearchChange(e.target.value)}
              className="w-full px-3 py-2 border border-slate-600 rounded-md bg-slate-700 text-slate-50 shadow-sm focus:outline-none focus:ring-blue-400 focus:border-blue-400 placeholder-slate-500"
            />
          </div>

          {/* Month */}
          {userType !== 'GERENTE LOJA' && userType !== 'GERENTE DEPARTAMENTO' && (
            <div>
              <label className="block text-sm font-medium text-slate-300 mb-2">
                Mês/Ano
              </label>
              <input
                type="month"
                value={filters.mesAno || ''}
                onChange={(e) => handleMesAnoChange(e.target.value)}
                className="w-full px-3 py-2 border border-slate-600 rounded-md bg-slate-700 text-slate-50 shadow-sm focus:outline-none focus:ring-blue-400 focus:border-blue-400"
              />
            </div>
          )}
        </div>

        {/* Radio buttons - Todos vs Próprios */}
        {(userType === 'VENDEDOR' || userType === 'VENDEDOR ESPECIAL') && (
          <div>
            <h3 className="text-sm font-semibold text-slate-50 mb-3">Filtro de Orçamentos</h3>
            <div className="space-y-2">
              <label className="flex items-center">
                <input
                  type="radio"
                  checked={!filters.apenasPropriosOrcamentos}
                  onChange={(e) => handlePropriosChange(!e.target.checked)}
                  className="h-4 w-4 text-blue-600 border-slate-600 bg-slate-700"
                />
                <span className="ml-2 text-sm text-slate-300">Todos os orçamentos</span>
              </label>
              <label className="flex items-center">
                <input
                  type="radio"
                  checked={filters.apenasPropriosOrcamentos}
                  onChange={(e) => handlePropriosChange(e.target.checked)}
                  className="h-4 w-4 text-blue-600 border-slate-600 bg-slate-700"
                />
                <span className="ml-2 text-sm text-slate-300">Apenas meus orçamentos</span>
              </label>
            </div>
          </div>
        )}
      </div>
    </div>
  );
}
