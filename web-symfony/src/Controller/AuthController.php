<?php

namespace App\Controller;

use App\Entity\Usuario;
use App\Service\AuthService;
use Doctrine\ORM\EntityManagerInterface;
use Symfony\Bundle\FrameworkBundle\Controller\AbstractController;
use Symfony\Component\HttpFoundation\Request;
use Symfony\Component\HttpFoundation\Response;
use Symfony\Component\Routing\Attribute\Route;
use Symfony\Component\Security\Http\Authentication\AuthenticationUtils;

class AuthController extends AbstractController
{
    private AuthService $authService;
    private EntityManagerInterface $entityManager;

    public function __construct(AuthService $authService, EntityManagerInterface $entityManager)
    {
        $this->authService = $authService;
        $this->entityManager = $entityManager;
    }

    /**
     * Login page - Display login form
     */
    #[Route('/', name: 'login', methods: ['GET', 'POST'])]
    public function login(Request $request): Response
    {
        error_log('[AUTH_CTRL] Login route accessed. Method: ' . $request->getMethod());

        // If already logged in, redirect to orcamentos
        if ($this->getUser()) {
            error_log('[AUTH_CTRL] User already logged in, redirecting to orcamentos');
            return $this->redirectToRoute('orcamentos_list');
        }

        $error = null;

        if ($request->isMethod('POST')) {
            error_log('[AUTH_CTRL] POST request received');
            $user = $request->request->get('user');
            $password = $request->request->get('password');
            $rememberPassword = $request->request->get('rememberPassword');

            error_log('[AUTH_CTRL] Form data - user: ' . $user . ', password: [' . strlen($password) . ' chars], remember: ' . ($rememberPassword ? 'true' : 'false'));

            try {
                error_log('[AUTH_CTRL] Calling authService->login()');
                // Authenticate using AuthService (MySQL SHA_PASSWORD)
                $result = $this->authService->login($user, $password);
                error_log('[AUTH_CTRL] AuthService->login() successful');

                // Load usuario entity
                error_log('[AUTH_CTRL] Loading usuario entity with ID: ' . $result['user']['idUsuario']);
                $usuario = $this->entityManager->getRepository(Usuario::class)->find($result['user']['idUsuario']);

                if ($usuario) {
                    error_log('[AUTH_CTRL] Usuario entity loaded, setting session data');
                    // Store in session
                    $request->getSession()->set('usuario_id', $usuario->getIdUsuario());
                    $request->getSession()->set('usuario_tipo', $usuario->getTipo());
                    $request->getSession()->set('usuario_idLoja', $usuario->getIdLoja());
                    $request->getSession()->set('usuario_nome', $usuario->getNome());
                    $request->getSession()->set('usuario_user', $usuario->getUser());

                    error_log('[AUTH_CTRL] Session data set. usuario_id: ' . $usuario->getIdUsuario());
                    error_log('[AUTH_CTRL] Session ID: ' . $request->getSession()->getId());

                    // Optionally store password if checkbox checked
                    if ($rememberPassword) {
                        error_log('[AUTH_CTRL] Remember password is checked, setting cookie');
                        $response = $this->redirectToRoute('orcamentos_list');
                        $response->headers->setCookie(\Symfony\Component\HttpFoundation\Cookie::create('rememberedUser')
                            ->withValue($user)
                            ->withExpires(time() + 30 * 24 * 60 * 60)
                        );
                        return $response;
                    }

                    error_log('[AUTH_CTRL] Redirecting to orcamentos_list');
                    return $this->redirectToRoute('orcamentos_list');
                } else {
                    error_log('[AUTH_CTRL] Usuario entity not found after successful login');
                    $error = 'Usuário não encontrado';
                }
            } catch (\RuntimeException $e) {
                error_log('[AUTH_CTRL] RuntimeException: ' . $e->getMessage());
                $error = $e->getMessage();
            } catch (\Exception $e) {
                error_log('[AUTH_CTRL] Unexpected exception: ' . get_class($e) . ' - ' . $e->getMessage());
                error_log('[AUTH_CTRL] Stack trace: ' . $e->getTraceAsString());
                $error = 'Erro ao processar login: ' . $e->getMessage();
            }
        }

        // Get remembered user from cookie
        $rememberedUser = $request->cookies->get('rememberedUser', '');
        error_log('[AUTH_CTRL] Rendering login form. Remembered user: ' . ($rememberedUser ?: 'none'));

        return $this->render('auth/login.html.twig', [
            'error' => $error,
            'remembered_user' => $rememberedUser,
        ]);
    }

    /**
     * Logout - Clear session and redirect to login
     */
    #[Route('/logout', name: 'logout', methods: ['GET'])]
    public function logout(Request $request): Response
    {
        $request->getSession()->clear();
        return $this->redirectToRoute('login');
    }
}
