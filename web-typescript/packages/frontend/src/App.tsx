import { useAuthStore } from './stores/auth.js';
import LoginDialog from './components/LoginDialog.js';
import { useState } from 'react';
import OrcamentoListPage from './pages/OrcamentoListPage.js';

type Page = 'dashboard' | 'orcamentos';

function App() {
  const { user, isAuthenticated } = useAuthStore();
  const [currentPage, setCurrentPage] = useState<Page>('dashboard');

  if (!isAuthenticated) {
    return <LoginDialog />;
  }

  return (
    <div className="min-h-screen bg-slate-950">
      <header className="bg-slate-900 shadow border-b border-slate-700">
        <div className="max-w-7xl mx-auto py-4 px-4 sm:px-6 lg:px-8 flex justify-between items-center">
          <h1 className="text-2xl font-bold text-slate-50">ERP Staccato</h1>
          <div className="flex items-center gap-4">
            <span className="text-slate-400">
              Olá, <strong>{user?.nome}</strong>
            </span>
            <span className="text-sm text-slate-500">({user?.tipo})</span>
            <button
              onClick={() => useAuthStore.getState().logout()}
              className="px-3 py-1 text-sm bg-red-600 text-white rounded hover:bg-red-700"
            >
              Sair
            </button>
          </div>
        </div>
      </header>

      {/* Navigation */}
      <nav className="bg-slate-900 border-b border-slate-700">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <div className="flex space-x-8">
            <button
              onClick={() => setCurrentPage('dashboard')}
              className={`py-4 px-1 border-b-2 font-medium text-sm ${
                currentPage === 'dashboard'
                  ? 'border-blue-400 text-blue-400'
                  : 'border-transparent text-slate-400 hover:text-slate-200 hover:border-slate-600'
              }`}
            >
              Dashboard
            </button>
            <button
              onClick={() => setCurrentPage('orcamentos')}
              className={`py-4 px-1 border-b-2 font-medium text-sm ${
                currentPage === 'orcamentos'
                  ? 'border-blue-400 text-blue-400'
                  : 'border-transparent text-slate-400 hover:text-slate-200 hover:border-slate-600'
              }`}
            >
              Orçamentos
            </button>
          </div>
        </div>
      </nav>

      {/* Page Content */}
      <main>
        {currentPage === 'dashboard' && (
          <div className="max-w-7xl mx-auto py-6 px-4 sm:px-6 lg:px-8">
            <div className="bg-slate-800 rounded-lg shadow p-6 border border-slate-700">
              <h2 className="text-xl font-semibold mb-4 text-slate-50">Bem-vindo ao ERP Staccato Web</h2>
              <p className="text-slate-400">
                Login realizado com sucesso. O sistema está em desenvolvimento.
              </p>
              <div className="mt-4 p-4 bg-slate-900 rounded border border-slate-700">
                <h3 className="font-medium mb-2 text-slate-50">Dados do usuário:</h3>
                <pre className="text-sm text-slate-300">
                  {JSON.stringify(user, null, 2)}
                </pre>
              </div>
            </div>
          </div>
        )}

        {currentPage === 'orcamentos' && <OrcamentoListPage />}
      </main>
    </div>
  );
}

export default App;
