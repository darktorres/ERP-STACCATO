import { useState, useMemo, useRef, useEffect } from 'react';
import { useAuthStore } from '../stores/auth.js';
import { trpc } from '../lib/trpc.js';
import { OrcamentoFilters } from '@erp-staccato/shared';
import { OrcamentoFilterPanel } from '../components/orcamento/OrcamentoFilterPanel.js';
import { OrcamentoTable } from '../components/orcamento/OrcamentoTable.js';

export default function OrcamentoListPage() {
  const user = useAuthStore((state) => state.user);

  // Initialize filters with role-based defaults
  const defaultFilters = useMemo(() => {
    const isAdmin = user?.tipo === 'ADMINISTRADOR' || user?.tipo === 'ADMINISTRATIVO';
    const isGerente = user?.tipo === 'GERENTE LOJA' || user?.tipo === 'GERENTE DEPARTAMENTO';
    const isVendedor = user?.tipo === 'VENDEDOR' || user?.tipo === 'VENDEDOR ESPECIAL';

    const filters: OrcamentoFilters = {
      statuses: ['ATIVO', 'EXPIRADO'],
    };

    // For ADMINISTRATIVO users, auto-enable month filter with current month
    if (isAdmin) {
      const today = new Date();
      const year = today.getFullYear();
      const month = String(today.getMonth() + 1).padStart(2, '0');
      filters.mesAno = `${year}-${month}`;
    }

    // For GERENTE users, show only their store (filter will be applied in backend)
    if (isGerente && user?.idLoja) {
      filters.idLoja = user.idLoja;
    }

    // For VENDEDOR users, default to showing only their budgets
    if (isVendedor) {
      filters.apenasPropriosOrcamentos = true;
    }

    return filters;
  }, [user]);

  const [filters, setFilters] = useState<OrcamentoFilters>(defaultFilters);
  const [selectedOrcamento, setSelectedOrcamento] = useState<any | null>(null);
  const [searchText, setSearchText] = useState('');
  const [statusEnabled, setStatusEnabled] = useState((defaultFilters.statuses || []).length > 0);
  const [mesEnabled, setMesEnabled] = useState(!!defaultFilters.mesAno);
  const searchTimeoutRef = useRef<NodeJS.Timeout | null>(null);

  // Debounce search
  useEffect(() => {
    if (searchTimeoutRef.current) {
      clearTimeout(searchTimeoutRef.current);
    }

    searchTimeoutRef.current = setTimeout(() => {
      setFilters((prevFilters) => ({ ...prevFilters, search: searchText || undefined }));
    }, 500);

    return () => {
      if (searchTimeoutRef.current) {
        clearTimeout(searchTimeoutRef.current);
      }
    };
  }, [searchText]);

  // Log when filters change
  useEffect(() => {
    console.log('[OrcamentoListPage] Filters changed:', {
      ...filters,
      mesAno: filters.mesAno ? `"${filters.mesAno}"` : undefined,
      mesEnabledState: mesEnabled,
    });
  }, [filters, mesEnabled]);

  // Queries with logging
  const { data: orcamentos, isLoading, isFetching } = trpc.orcamento.list.useQuery(filters, {
    onSuccess: (data) => {
      console.log(`[OrcamentoListPage] orcamento.list returned ${data?.length || 0} rows`);
    },
  });

  useEffect(() => {
    if (isLoading) {
      console.log('[OrcamentoListPage] orcamento.list → loading');
    } else if (isFetching) {
      console.log('[OrcamentoListPage] orcamento.list → refetching');
    } else if (orcamentos) {
      console.log('[OrcamentoListPage] orcamento.list → loaded with', orcamentos.length, 'rows');
    }
  }, [isLoading, isFetching, orcamentos]);

  const { data: lojas, isLoading: lojasLoading } = trpc.orcamento.lojas.useQuery(undefined, {
    enabled: !!user && user.tipo !== 'GERENTE LOJA' && user.tipo !== 'GERENTE DEPARTAMENTO',
  });
  const { data: vendedores, isLoading: vendedoresLoading } = trpc.orcamento.vendedores.useQuery(
    { idLoja: filters.idLoja },
    { enabled: !!user && user.tipo !== 'VENDEDOR' && user.tipo !== 'VENDEDOR ESPECIAL' }
  );
  const { data: fornecedores, isLoading: fornecedoresLoading } = trpc.orcamento.fornecedores.useQuery(undefined, {
    enabled: !!user,
  });

  return (
    <div className="flex h-[calc(100vh-120px)] bg-slate-950">
      {/* Left Sidebar */}
      <div className="w-48 bg-slate-900 border-r border-slate-700 p-4 overflow-y-auto flex flex-col">
        {/* Buttons */}
        <div className="space-y-2 mb-6">
          <button
            className="w-full px-4 py-2 bg-blue-600 text-white text-sm rounded hover:bg-blue-700 transition-colors"
            onClick={() => {
              console.log('Navigate to new orcamento');
            }}
          >
            Criar orçamento
          </button>
          <button
            disabled={!selectedOrcamento}
            className="w-full px-4 py-2 bg-amber-600 text-white text-sm rounded hover:bg-amber-700 transition-colors disabled:opacity-50 disabled:cursor-not-allowed"
            onClick={() => {
              if (!selectedOrcamento) {
                alert('Selecione um orçamento para abrir o followup');
                return;
              }
              console.log('Open followup for:', selectedOrcamento.idOrcamento);
              // TODO: Open FollowUp dialog
            }}
          >
            Followup
          </button>
        </div>

        {/* Filters Section */}
        <div className="space-y-4 flex-1">
          <h3 className="text-sm font-semibold text-slate-300 uppercase">Filtros</h3>

          {/* Todos/Próprios Radio */}
          {(user?.tipo === 'VENDEDOR' || user?.tipo === 'VENDEDOR ESPECIAL') && (
            <div className="space-y-2">
              <label className="flex items-center text-sm text-slate-300">
                <input
                  type="radio"
                  checked={!filters.apenasPropriosOrcamentos}
                  onChange={() => setFilters({ ...filters, apenasPropriosOrcamentos: false })}
                  className="h-4 w-4 text-blue-600 border-slate-600 bg-slate-700"
                />
                <span className="ml-2">Todos</span>
              </label>
              <label className="flex items-center text-sm text-slate-300">
                <input
                  type="radio"
                  checked={filters.apenasPropriosOrcamentos}
                  onChange={() => setFilters({ ...filters, apenasPropriosOrcamentos: true })}
                  className="h-4 w-4 text-blue-600 border-slate-600 bg-slate-700"
                />
                <span className="ml-2">Próprios</span>
              </label>
            </div>
          )}

          {/* Status Checkboxes */}
          <div>
            <label className="flex items-center text-xs font-semibold text-slate-400 uppercase mb-2">
              <input
                type="checkbox"
                checked={statusEnabled}
                onChange={(e) => {
                  setStatusEnabled(e.target.checked);
                  if (!e.target.checked) {
                    setFilters({ ...filters, statuses: [] });
                  }
                }}
                className="h-4 w-4 text-blue-600 border-slate-600 rounded bg-slate-700 mr-2"
              />
              Status
            </label>
            <div className="space-y-1">
              {['Ativo', 'Expirado', 'Cancelado', 'Fechado', 'Perdido', 'Replicado'].map((status) => (
                <label key={status} className="flex items-center text-sm text-slate-300">
                  <input
                    type="checkbox"
                    checked={(filters.statuses || []).includes(status.toUpperCase() as any)}
                    onChange={(e) => {
                      const statuses = filters.statuses || [];
                      const newStatuses = e.target.checked
                        ? [...statuses, status.toUpperCase() as any]
                        : statuses.filter((s) => s !== status.toUpperCase());
                      setFilters({ ...filters, statuses: newStatuses });
                    }}
                    disabled={!statusEnabled}
                    className="h-4 w-4 text-blue-600 border-slate-600 rounded bg-slate-700 disabled:opacity-50 disabled:cursor-not-allowed"
                  />
                  <span className={`ml-2 ${statusEnabled ? 'text-slate-300' : 'text-slate-500'}`}>{status}</span>
                </label>
              ))}
            </div>
          </div>

          {/* Month Filter */}
          {user?.tipo !== 'GERENTE LOJA' && user?.tipo !== 'GERENTE DEPARTAMENTO' && (
            <div>
              <label className="flex items-center text-xs font-semibold text-slate-400 uppercase mb-2">
                <input
                  type="checkbox"
                  checked={mesEnabled}
                  onChange={(e) => {
                    setMesEnabled(e.target.checked);
                    if (!e.target.checked) {
                      setFilters({ ...filters, mesAno: undefined });
                    }
                  }}
                  className="h-4 w-4 text-blue-600 border-slate-600 rounded bg-slate-700 mr-2"
                />
                Mês
              </label>
              <input
                type="month"
                value={filters.mesAno || ''}
                onChange={(e) => {
                  setFilters({ ...filters, mesAno: e.target.value || undefined });
                  if (e.target.value) {
                    setMesEnabled(true);
                  }
                }}
                disabled={!mesEnabled}
                className="w-full px-2 py-1 border border-slate-600 rounded-md bg-slate-700 text-slate-50 text-sm focus:outline-none focus:ring-blue-400 disabled:opacity-50 disabled:cursor-not-allowed"
              />
            </div>
          )}

          {/* Lojas Dropdown */}
          {user?.tipo !== 'GERENTE LOJA' && user?.tipo !== 'GERENTE DEPARTAMENTO' && (
            <div>
              <h4 className="text-xs font-semibold text-slate-400 uppercase mb-2">
                Lojas {lojasLoading && <span className="text-xs text-slate-500 font-normal ml-1">(carregando...)</span>}
              </h4>
              <select
                value={filters.idLoja || ''}
                onChange={(e) => setFilters({ ...filters, idLoja: e.target.value ? Number(e.target.value) : undefined })}
                disabled={lojasLoading}
                className="w-full px-2 py-1 border border-slate-600 rounded-md bg-slate-700 text-slate-50 text-sm focus:outline-none focus:ring-blue-400 disabled:opacity-50 disabled:cursor-not-allowed"
              >
                <option value="">{lojasLoading ? 'Carregando...' : 'Todas'}</option>
                {(lojas || []).map((loja) => (
                  <option key={loja.idLoja} value={loja.idLoja}>
                    {loja.descricao || loja.nomeFantasia}
                  </option>
                ))}
              </select>
            </div>
          )}

          {/* Vendedores Dropdown */}
          {user?.tipo !== 'VENDEDOR' && user?.tipo !== 'VENDEDOR ESPECIAL' && (
            <div>
              <h4 className="text-xs font-semibold text-slate-400 uppercase mb-2">
                Vendedores {vendedoresLoading && <span className="text-xs text-slate-500 font-normal ml-1">(carregando...)</span>}
              </h4>
              <select
                value={filters.idVendedor || ''}
                onChange={(e) => setFilters({ ...filters, idVendedor: e.target.value ? Number(e.target.value) : undefined })}
                disabled={vendedoresLoading}
                className="w-full px-2 py-1 border border-slate-600 rounded-md bg-slate-700 text-slate-50 text-sm focus:outline-none focus:ring-blue-400 disabled:opacity-50 disabled:cursor-not-allowed"
              >
                <option value="">{vendedoresLoading ? 'Carregando...' : 'Todos'}</option>
                {(vendedores || []).map((vendedor) => (
                  <option key={vendedor.idUsuario} value={vendedor.idUsuario}>
                    {vendedor.nome}
                  </option>
                ))}
              </select>
            </div>
          )}

          {/* Fornecedores Dropdown */}
          <div>
            <h4 className="text-xs font-semibold text-slate-400 uppercase mb-2">
              Fornecedores {fornecedoresLoading && <span className="text-xs text-slate-500 font-normal ml-1">(carregando...)</span>}
            </h4>
            <select
              value={filters.fornecedor || ''}
              onChange={(e) => setFilters({ ...filters, fornecedor: e.target.value || undefined })}
              disabled={fornecedoresLoading}
              className="w-full px-2 py-1 border border-slate-600 rounded-md bg-slate-700 text-slate-50 text-sm focus:outline-none focus:ring-blue-400 disabled:opacity-50 disabled:cursor-not-allowed"
            >
              <option value="">{fornecedoresLoading ? 'Carregando...' : 'Todos'}</option>
              {(fornecedores || []).map((fornecedor) => (
                <option key={fornecedor.razaoSocial} value={fornecedor.razaoSocial}>
                  {fornecedor.razaoSocial}
                </option>
              ))}
            </select>
          </div>

          {/* Followup Dropdown */}
          <div>
            <h4 className="text-xs font-semibold text-slate-400 uppercase mb-2">Followup</h4>
            <select
              value={filters.semaforo?.toString() || ''}
              onChange={(e) => setFilters({ ...filters, semaforo: e.target.value ? Number(e.target.value) : undefined })}
              className="w-full px-2 py-1 border border-slate-600 rounded-md bg-slate-700 text-slate-50 text-sm focus:outline-none focus:ring-blue-400"
            >
              <option value="">Todos</option>
              <option value="1">QUENTE</option>
              <option value="2">MORNO</option>
              <option value="3">FRIO</option>
            </select>
          </div>
        </div>
      </div>

      {/* Main Content Area */}
      <div className="flex-1 flex flex-col overflow-hidden">
        {/* Table */}
        {isLoading ? (
          <div className="flex-1 flex items-center justify-center">
            <div className="text-center">
              <p className="text-slate-400 mb-4">Carregando orçamentos...</p>
              <div className="inline-block animate-spin rounded-full h-8 w-8 border-b-2 border-blue-500"></div>
            </div>
          </div>
        ) : (
          <OrcamentoTable
            orcamentos={(Array.isArray(orcamentos) ? orcamentos : []) || []}
            selectedOrcamento={selectedOrcamento}
            onRowClick={(orcamento) => {
              setSelectedOrcamento(orcamento);
              console.log('Click row:', orcamento.idOrcamento);
            }}
          />
        )}

        {/* Search Bar */}
        <div className="bg-slate-900 border-t border-slate-700 p-4">
          <input
            type="text"
            placeholder="Buscar: Código/Vendedor/Cliente/Profissional"
            value={searchText}
            onChange={(e) => setSearchText(e.target.value)}
            className="w-full px-3 py-2 border border-slate-600 rounded-md bg-slate-700 text-slate-50 text-sm focus:outline-none focus:ring-blue-400 placeholder-slate-500"
          />
        </div>
      </div>
    </div>
  );
}
