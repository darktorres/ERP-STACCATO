<?php

namespace App\Controller;

use App\Entity\OrcamentoView;
use App\Entity\Usuario;
use App\Entity\Loja;
use Doctrine\ORM\EntityManagerInterface;
use Symfony\Bundle\FrameworkBundle\Controller\AbstractController;
use Symfony\Component\HttpFoundation\JsonResponse;
use Symfony\Component\HttpFoundation\Request;
use Symfony\Component\HttpFoundation\Response;
use Symfony\Component\Routing\Attribute\Route;

#[Route('/orcamentos')]
class OrcamentoController extends AbstractController
{
    private EntityManagerInterface $entityManager;

    public function __construct(EntityManagerInterface $entityManager)
    {
        $this->entityManager = $entityManager;
    }

    /**
     * Display quotation list page
     */
    #[Route('', name: 'orcamentos_list', methods: ['GET'])]
    public function list(Request $request): Response
    {
        error_log('[ORCAMENTOS] list() called');

        // Check if user is logged in
        $session = $request->getSession();
        error_log('[ORCAMENTOS] Session ID: ' . $session->getId());
        error_log('[ORCAMENTOS] Session has usuario_id: ' . ($session->has('usuario_id') ? 'YES' : 'NO'));

        if (!$session->has('usuario_id')) {
            error_log('[ORCAMENTOS] No usuario_id in session, redirecting to login');
            error_log('[ORCAMENTOS] Session data keys: ' . implode(', ', array_keys($session->all())));
            return $this->redirectToRoute('login');
        }

        // Get current user from session
        $usuarioId = $session->get('usuario_id');
        error_log('[ORCAMENTOS] usuario_id from session: ' . $usuarioId);
        $usuario = $this->entityManager->getRepository(Usuario::class)->find($usuarioId);

        if (!$usuario) {
            error_log('[ORCAMENTOS] Usuario not found in database for ID: ' . $usuarioId);
            $session->clear();
            return $this->redirectToRoute('login');
        }

        error_log('[ORCAMENTOS] Usuario loaded successfully: ' . $usuario->getNome());

        // Load filter options
        try {
            error_log('[ORCAMENTOS] Loading lojas...');
            $lojas = $this->entityManager->getRepository(Loja::class)
                ->findBy(['desativado' => false], ['idLoja' => 'ASC']);
            error_log('[ORCAMENTOS] Lojas loaded: ' . count($lojas) . ' active lojas');

            // Get initial data (unfiltered for sellers, filtered for managers)
            error_log('[ORCAMENTOS] Getting filtered orcamentos...');
            $initialOrcamentos = $this->getFilteredOrcamentos($usuario, []);
            error_log('[ORCAMENTOS] Initial orcamentos loaded: ' . count($initialOrcamentos) . ' records');
        } catch (\Exception $e) {
            error_log('[ORCAMENTOS] Exception while loading data: ' . $e->getMessage());
            error_log('[ORCAMENTOS] Stack trace: ' . $e->getTraceAsString());
            throw $e;
        }

        error_log('[ORCAMENTOS] Rendering quotation list template');

        return $this->render('quotation/list.html.twig', [
            'usuario' => $usuario,
            'lojas' => $lojas,
            'orcamentos' => $initialOrcamentos,
        ]);
    }

    /**
     * AJAX endpoint to get filtered quotations
     */
    #[Route('/data', name: 'orcamentos_data', methods: ['GET'])]
    public function getData(Request $request): JsonResponse
    {
        // Check if user is logged in
        $session = $request->getSession();
        if (!$session->has('usuario_id')) {
            return $this->json(['error' => 'Unauthorized'], Response::HTTP_UNAUTHORIZED);
        }

        // Get current user from session
        $usuarioId = $session->get('usuario_id');
        $usuario = $this->entityManager->getRepository(Usuario::class)->find($usuarioId);

        if (!$usuario) {
            return $this->json(['error' => 'Unauthorized'], Response::HTTP_UNAUTHORIZED);
        }

        try {
            // Get filters from request
            $filters = $request->query->all();

            // Get filtered data
            $orcamentos = $this->getFilteredOrcamentos($usuario, $filters);

            // Convert to array format
            $data = array_map(fn(OrcamentoView $o) => $o->toArray(), $orcamentos);

            return $this->json([
                'success' => true,
                'data' => $data,
            ]);
        } catch (\Exception $e) {
            return $this->json([
                'success' => false,
                'error' => $e->getMessage(),
            ], Response::HTTP_INTERNAL_SERVER_ERROR);
        }
    }

    /**
     * AJAX endpoint to get lojas dropdown
     */
    #[Route('/lojas', name: 'orcamentos_lojas', methods: ['GET'])]
    public function getLojas(Request $request): JsonResponse
    {
        $session = $request->getSession();
        if (!$session->has('usuario_id')) {
            return $this->json(['error' => 'Unauthorized'], Response::HTTP_UNAUTHORIZED);
        }

        $lojas = $this->entityManager->getRepository(Loja::class)
            ->findBy(['desativado' => false], ['idLoja' => 'ASC']);

        $result = array_map(fn(Loja $loja) => [
            'idLoja' => $loja->getIdLoja(),
            'descricao' => $loja->getDescricao(),
            'nomeFantasia' => $loja->getNomeFantasia(),
        ], $lojas);

        return $this->json($result);
    }

    /**
     * AJAX endpoint to get vendedores dropdown
     */
    #[Route('/vendedores', name: 'orcamentos_vendedores', methods: ['GET'])]
    public function getVendedores(Request $request): JsonResponse
    {
        $session = $request->getSession();
        if (!$session->has('usuario_id')) {
            return $this->json(['error' => 'Unauthorized'], Response::HTTP_UNAUTHORIZED);
        }

        $qb = $this->entityManager->getRepository(Usuario::class)
            ->createQueryBuilder('u')
            ->where('u.desativado = :desativado')
            ->andWhere('u.tipo IN (:tipos)')
            ->setParameter('desativado', false)
            ->setParameter('tipos', ['VENDEDOR', 'VENDEDOR ESPECIAL'])
            ->orderBy('u.nome', 'ASC');

        if ($request->query->has('idLoja')) {
            $qb->andWhere('u.idLoja = :idLoja')
                ->setParameter('idLoja', $request->query->get('idLoja'));
        }

        $vendedores = $qb->getQuery()->getResult();

        $result = array_map(fn(Usuario $u) => [
            'idUsuario' => $u->getIdUsuario(),
            'nome' => $u->getNome(),
        ], $vendedores);

        return $this->json($result);
    }

    /**
     * AJAX endpoint to get fornecedores dropdown
     */
    #[Route('/fornecedores', name: 'orcamentos_fornecedores', methods: ['GET'])]
    public function getFornecedores(Request $request): JsonResponse
    {
        $session = $request->getSession();
        if (!$session->has('usuario_id')) {
            return $this->json(['error' => 'Unauthorized'], Response::HTTP_UNAUTHORIZED);
        }

        $conn = $this->entityManager->getConnection();

        $sql = "SELECT DISTINCT fornecedores FROM view_orcamento
                WHERE fornecedores IS NOT NULL AND fornecedores != ''";

        $stmt = $conn->executeQuery($sql);
        $results = $stmt->fetchAllAssociative();

        // Parse comma-separated values
        $fornecedoresSet = [];
        foreach ($results as $row) {
            if ($row['fornecedores']) {
                $items = array_map('trim', explode(',', $row['fornecedores']));
                foreach ($items as $item) {
                    if ($item) {
                        $fornecedoresSet[$item] = true;
                    }
                }
            }
        }

        ksort($fornecedoresSet);

        $result = array_map(fn($razaoSocial) => ['razaoSocial' => $razaoSocial], array_keys($fornecedoresSet));

        return $this->json($result);
    }

    /**
     * Apply filters to quotations based on user role
     */
    private function getFilteredOrcamentos(Usuario $user, array $filters): array
    {
        $qb = $this->entityManager->createQueryBuilder()
            ->select('o')
            ->from(OrcamentoView::class, 'o');

        // Role-based filtering
        if ($user->isGerente() || $user->isAssistenteAdministrativo()) {
            // GERENTE LOJA/DEPARTAMENTO and ASSISTENTE ADMINISTRATIVO: Only their store
            $qb->where('o.idLoja = :idLoja')
                ->setParameter('idLoja', $user->getIdLoja());
        } elseif ($user->isVendedorOrEspecial()) {
            // VENDEDOR/VENDEDOR ESPECIAL: Own quotes if filter set
            if (!empty($filters['apenasPropriosOrcamentos'])) {
                $qb->andWhere('(o.vendedor = :nome OR o.consultor = :nome)')
                    ->setParameter('nome', $user->getNome());
            }
        } elseif (!$user->isAdmin() && !$user->isAdministrativo()) {
            // Others: filter by store if provided
            if (isset($filters['idLoja'])) {
                $qb->where('o.idLoja = :idLoja')
                    ->setParameter('idLoja', $filters['idLoja']);
            }
        }

        // Month filter
        if (!empty($filters['mesAno'])) {
            $qb->andWhere('o.data2 = :mesAno')
                ->setParameter('mesAno', $filters['mesAno']);
        }

        // Vendor filter
        if (!empty($filters['idVendedor'])) {
            $qb->andWhere('(o.idUsuario = :idVendedor OR o.idUsuarioConsultor = :idVendedor)')
                ->setParameter('idVendedor', $filters['idVendedor']);
        }

        // Supplier filter
        if (!empty($filters['fornecedor'])) {
            $fornecedor = $filters['fornecedor'];
            $qb->andWhere("FIND_IN_SET(:fornecedor, o.fornecedores) > 0")
                ->setParameter('fornecedor', $fornecedor);
        }

        // Status filter
        if (!empty($filters['statuses']) && is_array($filters['statuses'])) {
            $qb->andWhere('o.status IN (:statuses)')
                ->setParameter('statuses', $filters['statuses']);
        }

        // Semáforo filter
        if (isset($filters['semaforo']) && $filters['semaforo'] !== '') {
            $qb->andWhere('o.semaforo = :semaforo')
                ->setParameter('semaforo', (int)$filters['semaforo']);
        }

        // Text search
        if (!empty($filters['search'])) {
            $search = '%' . $filters['search'] . '%';
            $qb->andWhere('(
                o.idOrcamento LIKE :search OR
                o.vendedor LIKE :search OR
                o.cliente LIKE :search OR
                o.profissional LIKE :search
            )')
                ->setParameter('search', $search);
        }

        // Order by date DESC
        $qb->orderBy('o.data', 'DESC');

        // Limit initial load to 500 records for performance
        // AJAX requests can fetch more if needed
        error_log('[ORCAMENTOS] Query created, applying limit for initial load');
        if (empty($filters) || (count($filters) === 0 || !isset($filters['loadAll']))) {
            $qb->setMaxResults(500);
            error_log('[ORCAMENTOS] Limited to 500 records for initial page load');
        } else {
            error_log('[ORCAMENTOS] Loading all records (filters provided or loadAll flag set)');
        }

        return $qb->getQuery()->getResult();
    }
}
