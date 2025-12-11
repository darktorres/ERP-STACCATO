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
    <div className="min-h-screen bg-gray-100">
      <header className="bg-white shadow">
        <div className="max-w-7xl mx-auto py-4 px-4 sm:px-6 lg:px-8 flex justify-between items-center">
          <h1 className="text-2xl font-bold text-gray-900">ERP Staccato</h1>
          <div className="flex items-center gap-4">
            <span className="text-gray-600">
              Olá, <strong>{user?.nome}</strong>
            </span>
            <span className="text-sm text-gray-500">({user?.tipo})</span>
            <button
              onClick={() => useAuthStore.getState().logout()}
              className="px-3 py-1 text-sm bg-red-500 text-white rounded hover:bg-red-600"
            >
              Sair
            </button>
          </div>
        </div>
      </header>

      {/* Navigation */}
      <nav className="bg-white border-b border-gray-200">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <div className="flex space-x-8">
            <button
              onClick={() => setCurrentPage('dashboard')}
              className={`py-4 px-1 border-b-2 font-medium text-sm ${
                currentPage === 'dashboard'
                  ? 'border-blue-500 text-blue-600'
                  : 'border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300'
              }`}
            >
              Dashboard
            </button>
            <button
              onClick={() => setCurrentPage('orcamentos')}
              className={`py-4 px-1 border-b-2 font-medium text-sm ${
                currentPage === 'orcamentos'
                  ? 'border-blue-500 text-blue-600'
                  : 'border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300'
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
            <div className="bg-white rounded-lg shadow p-6">
              <h2 className="text-xl font-semibold mb-4">Bem-vindo ao ERP Staccato Web</h2>
              <p className="text-gray-600">
                Login realizado com sucesso. O sistema está em desenvolvimento.
              </p>
              <div className="mt-4 p-4 bg-gray-50 rounded">
                <h3 className="font-medium mb-2">Dados do usuário:</h3>
                <pre className="text-sm text-gray-600">
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
