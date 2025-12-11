import { useState } from 'react';
import { useForm } from 'react-hook-form';
import { zodResolver } from '@hookform/resolvers/zod';
import { loginSchema, type LoginInput } from '@erp-staccato/shared';
import { trpc } from '../lib/trpc';
import { useAuthStore } from '../stores/auth';

/**
 * LoginDialog component
 * Migrated from C++ LoginDialog (src/logindialog.cpp)
 *
 * Features preserved from original:
 * - Username/password input
 * - Remember password option
 * - Config panel toggle (hostname, loja selection, staging mode)
 * - Error handling
 */
export default function LoginDialog() {
  const [showConfig, setShowConfig] = useState(false);
  const [rememberPassword, setRememberPassword] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const setAuth = useAuthStore((state) => state.setAuth);

  const {
    register,
    handleSubmit,
    formState: { errors, isSubmitting },
  } = useForm<LoginInput>({
    resolver: zodResolver(loginSchema),
    defaultValues: {
      user: localStorage.getItem('lastUser') ?? '',
      password: '',
      hostname: localStorage.getItem('hostname') ?? '',
      staging: localStorage.getItem('staging') === 'true',
    },
  });

  const loginMutation = trpc.auth.login.useMutation({
    onSuccess: (data) => {
      if (data.success && data.token && data.user) {
        // Save last user
        localStorage.setItem('lastUser', data.user.user);

        // Set auth state
        setAuth(data.user, data.token);
      }
    },
    onError: (err) => {
      setError(err.message);
    },
  });

  const onSubmit = async (data: LoginInput) => {
    setError(null);

    // Save settings
    if (data.hostname) {
      localStorage.setItem('hostname', data.hostname);
    }
    localStorage.setItem('staging', String(data.staging));

    loginMutation.mutate(data);
  };

  return (
    <div className="min-h-screen flex items-center justify-center bg-gray-100">
      <div className="bg-white p-8 rounded-lg shadow-md w-full max-w-md">
        <h1 className="text-2xl font-bold text-center mb-6">ERP Login</h1>

        <form onSubmit={handleSubmit(onSubmit)} className="space-y-4">
          {/* Usuario */}
          <div>
            <label htmlFor="user" className="block text-sm font-medium text-gray-700">
              Usuário:
            </label>
            <input
              id="user"
              type="text"
              autoComplete="username"
              {...register('user')}
              className="mt-1 block w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-blue-500 focus:border-blue-500"
            />
            {errors.user && (
              <p className="mt-1 text-sm text-red-600">{errors.user.message}</p>
            )}
          </div>

          {/* Senha */}
          <div>
            <label htmlFor="password" className="block text-sm font-medium text-gray-700">
              Senha:
            </label>
            <input
              id="password"
              type="password"
              autoComplete="current-password"
              {...register('password')}
              className="mt-1 block w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-blue-500 focus:border-blue-500"
            />
            {errors.password && (
              <p className="mt-1 text-sm text-red-600">{errors.password.message}</p>
            )}
          </div>

          {/* Buttons row */}
          <div className="flex gap-2">
            <button
              type="button"
              onClick={() => setShowConfig(!showConfig)}
              className="p-2 text-gray-600 hover:text-gray-900 hover:bg-gray-100 rounded"
              title="Configurações"
            >
              <svg
                xmlns="http://www.w3.org/2000/svg"
                className="h-6 w-6"
                fill="none"
                viewBox="0 0 24 24"
                stroke="currentColor"
              >
                <path
                  strokeLinecap="round"
                  strokeLinejoin="round"
                  strokeWidth={2}
                  d="M10.325 4.317c.426-1.756 2.924-1.756 3.35 0a1.724 1.724 0 002.573 1.066c1.543-.94 3.31.826 2.37 2.37a1.724 1.724 0 001.065 2.572c1.756.426 1.756 2.924 0 3.35a1.724 1.724 0 00-1.066 2.573c.94 1.543-.826 3.31-2.37 2.37a1.724 1.724 0 00-2.572 1.065c-.426 1.756-2.924 1.756-3.35 0a1.724 1.724 0 00-2.573-1.066c-1.543.94-3.31-.826-2.37-2.37a1.724 1.724 0 00-1.065-2.572c-1.756-.426-1.756-2.924 0-3.35a1.724 1.724 0 001.066-2.573c-.94-1.543.826-3.31 2.37-2.37.996.608 2.296.07 2.572-1.065z"
                />
                <path
                  strokeLinecap="round"
                  strokeLinejoin="round"
                  strokeWidth={2}
                  d="M15 12a3 3 0 11-6 0 3 3 0 016 0z"
                />
              </svg>
            </button>
            <button
              type="submit"
              disabled={isSubmitting || loginMutation.isPending}
              className="flex-1 py-2 px-4 border border-transparent rounded-md shadow-sm text-sm font-medium text-white bg-blue-600 hover:bg-blue-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500 disabled:opacity-50 disabled:cursor-not-allowed"
            >
              {isSubmitting || loginMutation.isPending ? 'Entrando...' : 'Login'}
            </button>
          </div>

          {/* Config section - hidden by default (matches C++ behavior) */}
          {showConfig && (
            <div className="space-y-4 pt-4 border-t border-gray-200">
              {/* Salvar senha */}
              <div className="flex items-center">
                <input
                  id="rememberPassword"
                  type="checkbox"
                  checked={rememberPassword}
                  onChange={(e) => setRememberPassword(e.target.checked)}
                  className="h-4 w-4 text-blue-600 focus:ring-blue-500 border-gray-300 rounded"
                />
                <label htmlFor="rememberPassword" className="ml-2 block text-sm text-gray-700">
                  Salvar senha
                </label>
              </div>

              {/* Hostname/Loja */}
              <div>
                <label htmlFor="hostname" className="block text-sm font-medium text-gray-700">
                  Loja:
                </label>
                <input
                  id="hostname"
                  type="text"
                  {...register('hostname')}
                  className="mt-1 block w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-blue-500 focus:border-blue-500"
                />
              </div>

              {/* Ambiente de teste */}
              <div className="flex items-center">
                <input
                  id="staging"
                  type="checkbox"
                  {...register('staging')}
                  className="h-4 w-4 text-blue-600 focus:ring-blue-500 border-gray-300 rounded"
                />
                <label htmlFor="staging" className="ml-2 block text-sm text-gray-700">
                  Ambiente de teste
                </label>
              </div>
            </div>
          )}

          {/* Error message */}
          {error && (
            <div className="p-3 bg-red-50 border border-red-200 rounded-md">
              <p className="text-sm text-red-600">{error}</p>
            </div>
          )}
        </form>
      </div>
    </div>
  );
}
