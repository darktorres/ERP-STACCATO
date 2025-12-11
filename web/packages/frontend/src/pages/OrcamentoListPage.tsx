import { useState } from 'react';
import { useAuthStore } from '../stores/auth.js';
import { trpc } from '../lib/trpc.js';
import { OrcamentoFilters } from '@erp-staccato/shared';
import { OrcamentoFilterPanel } from '../components/orcamento/OrcamentoFilterPanel.js';
import { OrcamentoTable } from '../components/orcamento/OrcamentoTable.js';

export default function OrcamentoListPage() {
  const user = useAuthStore((state) => state.user);
  const [filters, setFilters] = useState<OrcamentoFilters>({
    apenasPropriosOrcamentos: user?.tipo === 'VENDEDOR' || user?.tipo === 'VENDEDOR ESPECIAL',
    statuses: ['ATIVO', 'EXPIRADO'],
  });
  const [selectedOrcamento, setSelectedOrcamento] = useState<any | null>(null);

  // Queries
  const { data: orcamentos, isLoading } = trpc.orcamento.list.useQuery(filters);
  const { data: lojas } = trpc.orcamento.lojas.useQuery(undefined, {
    enabled: user?.tipo !== 'GERENTE LOJA',
  });
  const { data: vendedores } = trpc.orcamento.vendedores.useQuery(
    { idLoja: filters.idLoja },
    { enabled: true }
  );
  const { data: fornecedores } = trpc.orcamento.fornecedores.useQuery();

  return (
    <div className="min-h-screen bg-slate-950 p-6">
      <div className="max-w-7xl mx-auto">
        {/* Header */}
        <div className="flex justify-between items-center mb-6">
          <h1 className="text-3xl font-bold text-slate-50">Orçamentos</h1>
          <div className="flex gap-3">
            <button
              disabled={!selectedOrcamento}
              className="px-4 py-2 bg-amber-600 text-white rounded-lg hover:bg-amber-700 transition-colors disabled:opacity-50 disabled:cursor-not-allowed"
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
            <button
              className="px-4 py-2 bg-blue-600 text-white rounded-lg hover:bg-blue-700 transition-colors"
              onClick={() => {
                // Navigate to new orcamento page
                console.log('Navigate to new orcamento');
              }}
            >
              + Criar Orçamento
            </button>
          </div>
        </div>

        {/* Filters */}
        <OrcamentoFilterPanel
          filters={filters}
          onFiltersChange={setFilters}
          lojas={(lojas || []).map((loja) => ({
            idLoja: loja.idLoja,
            descricao: loja.descricao ?? undefined,
            nomeFantasia: loja.nomeFantasia,
          }))}
          vendedores={vendedores || []}
          fornecedores={fornecedores || []}
          userType={user?.tipo}
        />

        {/* Table */}
        {isLoading ? (
          <div className="text-center py-8">
            <p className="text-slate-400">Carregando orçamentos...</p>
          </div>
        ) : (
          <OrcamentoTable
            orcamentos={(Array.isArray(orcamentos) ? orcamentos : []) || []}
            selectedOrcamento={selectedOrcamento}
            onRowClick={(orcamento) => {
              setSelectedOrcamento(orcamento);
              console.log('Click row:', orcamento.idOrcamento);
              // Navigate to orcamento detail page
            }}
          />
        )}
      </div>
    </div>
  );
}
