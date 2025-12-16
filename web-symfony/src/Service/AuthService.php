<?php

namespace App\Service;

use App\Entity\Usuario;
use App\Entity\Maintenance;
use Doctrine\ORM\EntityManagerInterface;
use Lcobucci\JWT\Configuration;
use Lcobucci\JWT\Signer\Hmac\Sha256;
use Lcobucci\JWT\Signer\Key\InMemory;

class AuthService
{
    private EntityManagerInterface $entityManager;
    private Configuration $jwtConfig;

    public function __construct(EntityManagerInterface $entityManager, string $jwtSecret)
    {
        $this->entityManager = $entityManager;

        // Configure JWT with HMAC SHA256
        $this->jwtConfig = Configuration::forSymmetricSigner(
            new Sha256(),
            InMemory::plainText($jwtSecret)
        );
    }

    /**
     * Login endpoint logic matching C++ LoginDialog
     *
     * @throws \RuntimeException
     */
    public function login(string $user, string $password): array
    {
        error_log('[LOGIN] Starting login attempt for user: ' . $user);

        // Check maintenance mode
        error_log('[LOGIN] Checking maintenance mode...');
        $maintenance = $this->entityManager->getRepository(Maintenance::class)->find(1);
        if ($maintenance && $maintenance->isEmManutencao()) {
            error_log('[LOGIN] System is in maintenance mode');
            throw new \RuntimeException('Sistema em manutenção!');
        }
        error_log('[LOGIN] Maintenance check passed');

        // Query user with SHA_PASSWORD verification
        // Using raw SQL because Doctrine doesn't natively support MySQL functions
        $conn = $this->entityManager->getConnection();

        $sql = <<<SQL
            SELECT u.idUsuario, u.idLoja, u.nome, u.tipo, l.descricao, l.nomeFantasia
            FROM usuario u
            LEFT JOIN loja l ON u.idLoja = l.idLoja
            WHERE UPPER(u.user) = UPPER(?)
            AND u.password = SHA_PASSWORD(?)
            AND u.desativado = FALSE
        SQL;

        try {
            error_log('[LOGIN] Executing SQL query for user: ' . $user);
            $stmt = $conn->executeQuery($sql, [$user, $password]);
            $result = $stmt->fetchAssociative();
            error_log('[LOGIN] Query result: ' . ($result ? 'FOUND' : 'NOT FOUND'));
            if ($result) {
                error_log('[LOGIN] Result data: idUsuario=' . $result['idUsuario'] . ', tipo=' . $result['tipo']);
            }
        } catch (\Exception $e) {
            error_log('[LOGIN] SQL Exception: ' . $e->getMessage());
            throw new \RuntimeException('Erro ao consultar usuário: ' . $e->getMessage());
        }

        if (!$result) {
            error_log('[LOGIN] User not found or password incorrect');
            throw new \RuntimeException('Login inválido!');
        }

        // Block OPERACIONAL users
        if ($result['tipo'] === 'OPERACIONAL') {
            error_log('[LOGIN] User is OPERACIONAL type - blocking');
            throw new \RuntimeException('Operacional bloqueado!');
        }

        // Load the full usuario entity
        error_log('[LOGIN] Loading usuario entity with ID: ' . $result['idUsuario']);
        $usuario = $this->entityManager->getRepository(Usuario::class)->find($result['idUsuario']);
        if (!$usuario) {
            error_log('[LOGIN] Failed to load usuario entity');
            throw new \RuntimeException('Usuário não encontrado');
        }
        error_log('[LOGIN] Usuario entity loaded successfully');

        // Generate JWT token
        $token = $this->generateToken($usuario);

        error_log('[LOGIN] Login successful for user: ' . $user . ' (ID: ' . $usuario->getIdUsuario() . ')');

        return [
            'success' => true,
            'token' => $token->toString(),
            'user' => [
                'idUsuario' => $usuario->getIdUsuario(),
                'idLoja' => $usuario->getIdLoja(),
                'user' => strtolower($usuario->getUser()),
                'nome' => $usuario->getNome(),
                'tipo' => $usuario->getTipo(),
                'loja' => $usuario->getLoja() ? [
                    'idLoja' => $usuario->getLoja()->getIdLoja(),
                    'descricao' => $usuario->getLoja()->getDescricao(),
                    'nomeFantasia' => $usuario->getLoja()->getNomeFantasia(),
                ] : null,
            ],
        ];
    }

    /**
     * Get authenticated user info
     */
    public function getUserInfo(Usuario $usuario): array
    {
        return [
            'idUsuario' => $usuario->getIdUsuario(),
            'idLoja' => $usuario->getIdLoja(),
            'user' => strtolower($usuario->getUser()),
            'nome' => $usuario->getNome(),
            'tipo' => $usuario->getTipo(),
            'loja' => $usuario->getLoja() ? [
                'idLoja' => $usuario->getLoja()->getIdLoja(),
                'descricao' => $usuario->getLoja()->getDescricao(),
                'nomeFantasia' => $usuario->getLoja()->getNomeFantasia(),
            ] : null,
        ];
    }

    /**
     * Generate JWT token for usuario
     */
    private function generateToken(Usuario $usuario)
    {
        $now = new \DateTimeImmutable();

        return $this->jwtConfig->builder()
            ->issuedAt($now)
            ->expiresAt($now->modify('+24 hours'))
            ->withClaim('idUsuario', $usuario->getIdUsuario())
            ->withClaim('idLoja', $usuario->getIdLoja())
            ->withClaim('tipo', $usuario->getTipo())
            ->withClaim('user', strtolower($usuario->getUser()))
            ->withClaim('nome', $usuario->getNome())
            ->getToken($this->jwtConfig->signer(), $this->jwtConfig->signingKey());
    }

    /**
     * Parse and validate JWT token
     */
    public function validateToken(string $tokenString)
    {
        try {
            $token = $this->jwtConfig->parser()->parse($tokenString);

            // Validate signature and expiration
            if (!$token->verify($this->jwtConfig->signer(), $this->jwtConfig->signingKey())) {
                throw new \RuntimeException('Invalid token signature');
            }

            // Check expiration
            if ($token->isExpired(new \DateTimeImmutable())) {
                throw new \RuntimeException('Token expired');
            }

            return $token;
        } catch (\Exception $e) {
            throw new \RuntimeException('Invalid token: ' . $e->getMessage());
        }
    }

    /**
     * Get user from token claims
     */
    public function getUserFromToken($token): ?Usuario
    {
        $idUsuario = $token->claims()->get('idUsuario');
        if (!$idUsuario) {
            return null;
        }

        return $this->entityManager->getRepository(Usuario::class)->find($idUsuario);
    }
}
