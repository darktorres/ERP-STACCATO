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
 * Features:
 * - Username/password input
 * - Remember password option
 * - Error handling
 */
export default function LoginDialog() {
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

          {/* Options */}
          <div className="space-y-3 pt-2">
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

          {/* Login button */}
          <button
            type="submit"
            disabled={isSubmitting || loginMutation.isPending}
            className="w-full py-2 px-4 border border-transparent rounded-md shadow-sm text-sm font-medium text-white bg-blue-600 hover:bg-blue-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500 disabled:opacity-50 disabled:cursor-not-allowed"
          >
            {isSubmitting || loginMutation.isPending ? 'Entrando...' : 'Login'}
          </button>

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
