# Staccato ERP - Validation Comparison Across Implementations

## Overview
Comprehensive comparison of validation strategies used in each implementation for login, quotations filtering, and data integrity.

---

## Login Form Validation

### C++ Qt (Desktop)

**Frontend Validation:**
```cpp
// QLineEdit validators
QLineEdit *userInput = new QLineEdit();
QLineEdit *passwordInput = new QLineEdit();

// Input masks and length validation
userInput->setMaxLength(45);
passwordInput->setMaxLength(255);
passwordInput->setEchoMode(QLineEdit::Password);

// Custom validator
class UsernameValidator : public QValidator {
    State validate(QString &input, int &pos) const override {
        if (input.isEmpty()) return Intermediate;
        if (input.length() > 45) return Invalid;
        // Allow alphanumeric and underscore
        QRegExp rx("^[a-zA-Z0-9_]*$");
        if (!rx.exactMatch(input)) return Invalid;
        return Acceptable;
    }
};
```

**Backend Validation:**
```cpp
// In AuthService
void AuthService::validateCredentials(const QString &user, const QString &password) {
    // Check null/empty
    if (user.isEmpty() || password.isEmpty()) {
        throw std::runtime_error("Username and password required");
    }

    // Check length
    if (user.length() > 45) {
        throw std::runtime_error("Username too long");
    }
    if (password.length() > 255) {
        throw std::runtime_error("Password too long");
    }

    // SQL injection protection (parameterized queries)
    QSqlQuery query;
    query.prepare("SELECT * FROM usuario WHERE user = ? AND password = SHA_PASSWORD(?)");
    query.addBindValue(user);
    query.addBindValue(password);
    if (!query.exec()) {
        throw std::runtime_error("Database error: " + query.lastError().text().toStdString());
    }
}
```

**Validation Rules:**
- ✅ Non-empty username and password
- ✅ Username max 45 characters
- ✅ Password max 255 characters
- ✅ SQL injection protection (parameterized queries)
- ✅ User account must exist and not be disabled
- ✅ User type cannot be OPERACIONAL

---

### .NET ASP.NET (web-dotnet)

**Frontend Validation:**
```csharp
// HTML5 validation attributes
<form method="POST">
    <input type="text" name="username"
           required maxlength="45"
           pattern="[a-zA-Z0-9_]+" />
    <input type="password" name="password"
           required maxlength="255" />
</form>

// Client-side JavaScript (optional)
document.querySelectorAll('form').forEach(form => {
    form.addEventListener('submit', (e) => {
        const username = form.querySelector('[name="username"]').value;
        if (!/^[a-zA-Z0-9_]+$/.test(username)) {
            e.preventDefault();
            alert('Invalid username format');
        }
    });
});
```

**Backend Validation:**
```csharp
[HttpPost]
public async Task<IActionResult> Login(LoginViewModel model)
{
    // ModelState validation (data annotations)
    if (!ModelState.IsValid)
    {
        ModelState.AddModelError("", "Username and password required");
        return View(model);
    }

    // Business logic validation
    var user = await _userManager.FindByNameAsync(model.Username);
    if (user == null)
    {
        ModelState.AddModelError("", "Invalid username or password");
        return View(model);
    }

    // Check if user is disabled
    if (!user.IsActive)
    {
        ModelState.AddModelError("", "User account is disabled");
        return View(model);
    }

    // Check user type
    if (user.UserType == UserType.OPERACIONAL)
    {
        ModelState.AddModelError("", "OPERACIONAL users cannot log in");
        return View(model);
    }

    // Verify password (hashed comparison)
    var result = await _userManager.CheckPasswordAsync(user, model.Password);
    if (!result)
    {
        ModelState.AddModelError("", "Invalid username or password");
        return View(model);
    }

    // Check maintenance mode
    var maintenance = await _dbContext.MaintenanceSettings.FirstOrDefaultAsync();
    if (maintenance?.IsUnderMaintenance == true)
    {
        ModelState.AddModelError("", "System is under maintenance");
        return View(model);
    }

    return RedirectToAction("Dashboard");
}
```

**Validation Rules:**
- ✅ DataAnnotations ([Required], [StringLength], [RegularExpression])
- ✅ Non-empty fields
- ✅ Username max 45 characters
- ✅ Password max 255 characters
- ✅ Username format validation (alphanumeric + underscore)
- ✅ User existence check
- ✅ Account enabled status
- ✅ User type restrictions
- ✅ Maintenance mode check
- ✅ Password hashing verification (bcrypt)

---

### Laravel (web-laravel)

**Frontend Validation:**
```blade
<!-- Blade template with HTML5 validation -->
<form method="POST" action="{{ route('login') }}">
    @csrf
    <input type="text" name="user"
           required maxlength="45"
           pattern="[a-zA-Z0-9_]+"
           value="{{ old('user') }}" />
    @error('user')
        <span class="error">{{ $message }}</span>
    @enderror

    <input type="password" name="password"
           required maxlength="255" />
    @error('password')
        <span class="error">{{ $message }}</span>
    @enderror
</form>
```

**Backend Validation:**
```php
// LoginRequest - Form Request Validation
namespace App\Http\Requests;

class LoginRequest extends FormRequest
{
    public function authorize()
    {
        return true;
    }

    public function rules()
    {
        return [
            'user' => 'required|string|max:45|regex:/^[a-zA-Z0-9_]+$/',
            'password' => 'required|string|max:255',
        ];
    }

    public function messages()
    {
        return [
            'user.required' => 'Username is required',
            'user.max' => 'Username cannot exceed 45 characters',
            'user.regex' => 'Username must contain only letters, numbers, and underscores',
            'password.required' => 'Password is required',
            'password.max' => 'Password cannot exceed 255 characters',
        ];
    }
}

// LoginController
public function store(LoginRequest $request)
{
    // Request is already validated by LoginRequest
    $user = Usuario::where('user', strtolower($request->user))
                    ->where('desativado', false)
                    ->first();

    if (!$user) {
        return back()->withErrors(['user' => 'Invalid username or password']);
    }

    // Check user type
    if ($user->tipo === 'OPERACIONAL') {
        return back()->withErrors(['user' => 'OPERACIONAL users cannot log in']);
    }

    // Verify password (SHA_PASSWORD for compatibility)
    $hashedPassword = hash('sha1', $request->password);
    if ($user->password !== $hashedPassword) {
        return back()->withErrors(['password' => 'Invalid username or password']);
    }

    // Check maintenance mode
    $maintenance = Maintenance::first();
    if ($maintenance && $maintenance->em_manutencao) {
        return back()->withErrors(['user' => 'System is under maintenance']);
    }

    // Create session
    session(['user_id' => $user->id_usuario, 'user' => $user]);

    return redirect()->route('quotations.index');
}
```

**Validation Rules:**
- ✅ Form Request validation (auto-validated before controller)
- ✅ Required fields
- ✅ Max length constraints
- ✅ Regular expression pattern matching
- ✅ Custom error messages
- ✅ User existence and status check
- ✅ User type validation
- ✅ Maintenance mode check
- ✅ Password verification

---

### Symfony (web-symfony) - Recently Completed

**Frontend Validation:**
```html
<!-- Twig template with HTML5 validation -->
<form method="POST" id="loginForm">
    <input type="text"
           id="user"
           name="user"
           required
           maxlength="45"
           pattern="[a-zA-Z0-9_]+"
           placeholder="Digite seu usuário" />

    <input type="password"
           id="password"
           name="password"
           required
           maxlength="255"
           placeholder="Digite sua senha" />
</form>

<!-- JavaScript validation (optional) -->
<script>
document.getElementById('loginForm').addEventListener('submit', function(e) {
    const user = document.getElementById('user').value;
    const password = document.getElementById('password').value;

    if (!user || !password) {
        e.preventDefault();
        alert('Username and password are required');
        return false;
    }

    if (user.length > 45) {
        e.preventDefault();
        alert('Username cannot exceed 45 characters');
        return false;
    }

    if (!/^[a-zA-Z0-9_]+$/.test(user)) {
        e.preventDefault();
        alert('Username must contain only letters, numbers, and underscores');
        return false;
    }
});
</script>
```

**Backend Validation:**
```php
// AuthController - Login validation
public function login(Request $request): Response
{
    // HTML5 validation provides client-side feedback
    // Server-side validation is critical for security

    $user = $request->request->get('user');
    $password = $request->request->get('password');

    // Validate input presence
    if (empty($user) || empty($password)) {
        $error = 'Username and password are required';
        return $this->render('auth/login.html.twig', ['error' => $error]);
    }

    // Validate input length
    if (strlen($user) > 45 || strlen($password) > 255) {
        $error = 'Invalid input length';
        return $this->render('auth/login.html.twig', ['error' => $error]);
    }

    // Validate username format
    if (!preg_match('/^[a-zA-Z0-9_]+$/', $user)) {
        $error = 'Invalid username format';
        return $this->render('auth/login.html.twig', ['error' => $error]);
    }

    try {
        // AuthService handles authentication with validation
        $result = $this->authService->login($user, $password);
        // ... rest of login logic
    } catch (\RuntimeException $e) {
        // Catch validation errors from AuthService
        $error = $e->getMessage();
        return $this->render('auth/login.html.twig', ['error' => $error]);
    }
}

// AuthService - Business logic validation
public function login(string $user, string $password): array
{
    // Maintenance mode check
    $maintenance = $this->entityManager->getRepository(Maintenance::class)->find(1);
    if ($maintenance && $maintenance->isEmManutencao()) {
        throw new \RuntimeException('Sistema em manutenção!');
    }

    // Query with parameterized SQL for injection protection
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
        $stmt = $conn->executeQuery($sql, [$user, $password]);
        $result = $stmt->fetchAssociative();
    } catch (\Exception $e) {
        throw new \RuntimeException('Database error: ' . $e->getMessage());
    }

    if (!$result) {
        throw new \RuntimeException('Login inválido!');
    }

    // Block OPERACIONAL users
    if ($result['tipo'] === 'OPERACIONAL') {
        throw new \RuntimeException('Operacional bloqueado!');
    }

    // Load usuario entity
    $usuario = $this->entityManager->getRepository(Usuario::class)->find($result['idUsuario']);
    if (!$usuario) {
        throw new \RuntimeException('Usuário não encontrado');
    }

    return [
        'success' => true,
        'user' => [...],
    ];
}
```

**Validation Rules:**
- ✅ HTML5 form validation (required, maxlength, pattern)
- ✅ JavaScript client-side validation
- ✅ Server-side presence checks
- ✅ Length validation
- ✅ Format validation (regex pattern)
- ✅ Parameterized SQL queries (SQL injection prevention)
- ✅ Maintenance mode check
- ✅ User type validation
- ✅ Account active status check
- ✅ Error logging

---

### TypeScript/Node.js (web-typescript)

**Frontend Validation:**
```typescript
// React/Vue component validation
interface LoginForm {
  username: string;
  password: string;
}

const loginSchema = z.object({
  username: z.string()
    .min(1, 'Username is required')
    .max(45, 'Username cannot exceed 45 characters')
    .regex(/^[a-zA-Z0-9_]+$/, 'Invalid username format'),
  password: z.string()
    .min(1, 'Password is required')
    .max(255, 'Password cannot exceed 255 characters'),
});

// Form component
export const LoginForm: React.FC = () => {
  const [errors, setErrors] = useState<Record<string, string>>({});

  const handleSubmit = async (data: LoginForm) => {
    try {
      // Client-side validation
      loginSchema.parse(data);
      setErrors({});

      // Send to backend
      const response = await fetch('/api/auth/login', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data),
      });

      if (!response.ok) {
        const error = await response.json();
        setErrors(error.errors || { general: error.message });
      }
    } catch (error) {
      if (error instanceof z.ZodError) {
        const newErrors: Record<string, string> = {};
        error.errors.forEach(err => {
          newErrors[err.path[0] as string] = err.message;
        });
        setErrors(newErrors);
      }
    }
  };

  return (
    <form onSubmit={handleSubmit}>
      <input type="text" name="username" required maxLength={45} />
      {errors.username && <span className="error">{errors.username}</span>}

      <input type="password" name="password" required maxLength={255} />
      {errors.password && <span className="error">{errors.password}</span>}
    </form>
  );
};
```

**Backend Validation:**
```typescript
// Express middleware for validation
import { Router } from 'express';
import { z } from 'zod';

const loginRouter = Router();

const loginSchema = z.object({
  username: z.string()
    .trim()
    .min(1, 'Username is required')
    .max(45, 'Username cannot exceed 45 characters')
    .regex(/^[a-zA-Z0-9_]+$/, 'Invalid username format'),
  password: z.string()
    .min(1, 'Password is required')
    .max(255, 'Password cannot exceed 255 characters'),
});

// Validation middleware
const validateLogin = (req: Request, res: Response, next: NextFunction) => {
  try {
    req.body = loginSchema.parse(req.body);
    next();
  } catch (error) {
    if (error instanceof z.ZodError) {
      return res.status(400).json({ errors: error.flatten().fieldErrors });
    }
    res.status(400).json({ message: 'Invalid request' });
  }
};

loginRouter.post('/login', validateLogin, async (req: Request, res: Response) => {
  const { username, password } = req.body;

  // Database query with parameterized statement
  const user = await db.query(
    'SELECT * FROM usuario WHERE UPPER(user) = UPPER($1) AND desativado = FALSE',
    [username]
  );

  if (!user.rows.length) {
    return res.status(401).json({ message: 'Invalid username or password' });
  }

  // Check user type
  if (user.rows[0].tipo === 'OPERACIONAL') {
    return res.status(403).json({ message: 'OPERACIONAL users cannot log in' });
  }

  // Verify password (bcrypt comparison)
  const validPassword = await bcrypt.compare(password, user.rows[0].password);
  if (!validPassword) {
    return res.status(401).json({ message: 'Invalid username or password' });
  }

  // Check maintenance mode
  const maintenance = await db.query('SELECT * FROM maintenance WHERE id = $1', [1]);
  if (maintenance.rows[0]?.em_manutencao) {
    return res.status(503).json({ message: 'System is under maintenance' });
  }

  // Create JWT token
  const token = jwt.sign(
    { id: user.rows[0].id_usuario, username: user.rows[0].user },
    process.env.JWT_SECRET,
    { expiresIn: '24h' }
  );

  res.json({ token, user: user.rows[0] });
});
```

**Validation Rules:**
- ✅ Zod schema validation (TypeScript-first)
- ✅ Validation middleware
- ✅ Required fields
- ✅ Max length constraints
- ✅ Pattern matching (regex)
- ✅ Parameterized database queries
- ✅ User existence check
- ✅ User type validation
- ✅ Maintenance mode check
- ✅ Password hashing verification (bcrypt)
- ✅ Detailed error messages

---

## Quotations Filter Validation

### C++ Qt

**Filter Input Validation:**
```cpp
// Status checkboxes - no validation needed (binary state)
void WidgetOrcamento::on_checkBoxFechado_toggled(bool checked) {
    // Direct boolean handling
    this->montaFiltro();
}

// Date filter validation
void WidgetOrcamento::on_dateEditMes_dateChanged(const QDate &date) {
    // QDateEdit ensures valid date
    if (date.isValid()) {
        QString mesAno = date.toString("yyyy-MM");
        this->montaFiltro();
    }
}

// Combo box validation (vendor dropdown)
void WidgetOrcamento::on_comboBoxVendedor_currentTextChanged(const QString &text) {
    // Combo box restricts to pre-loaded values
    if (!text.isEmpty() && text != "Todos") {
        this->montaFiltro();
    }
}

// Search input with delayed response
class LineEdit : public QLineEdit {
    void onTextChanged(const QString &text) {
        // Validate search text length
        if (text.length() <= 1000) {  // Reasonable limit
            emit delayedTextChanged();
        }
    }
};
```

**Validation Logic:**
- ✅ Checkbox states (binary)
- ✅ Date validation (QDateEdit ensures valid dates)
- ✅ Combo box constraint (only predefined values)
- ✅ Search text length limit (1000 chars)
- ✅ Numeric validation for semáforo (0-3)

---

### .NET ASP.NET

**Filter Model Validation:**
```csharp
public class QuotationFilterModel
{
    [Range(1, int.MaxValue)]
    public int? StoreId { get; set; }

    [Range(1, int.MaxValue)]
    public int? VendorId { get; set; }

    [StringLength(100)]
    [RegularExpression(@"^[a-zA-Z0-9\s\-áéíóúãõç]*$",
        ErrorMessage = "Search contains invalid characters")]
    public string SearchText { get; set; }

    [RegularExpression(@"^\d{4}-\d{2}$",
        ErrorMessage = "Invalid month format")]
    public string MonthYear { get; set; }

    [Range(0, 3)]
    public int? Semaphore { get; set; }
}

[HttpGet]
public async Task<IActionResult> GetQuotations([FromQuery] QuotationFilterModel filter)
{
    // ModelState validation
    if (!ModelState.IsValid)
    {
        return BadRequest(ModelState);
    }

    // Validate dates if provided
    if (!string.IsNullOrEmpty(filter.MonthYear))
    {
        if (!DateTime.TryParseExact(filter.MonthYear + "-01",
            "yyyy-MM-dd", CultureInfo.InvariantCulture,
            DateTimeStyles.None, out var date))
        {
            ModelState.AddModelError("monthYear", "Invalid month format");
            return BadRequest(ModelState);
        }
    }

    // Retrieve and filter data
    var quotations = await _dbContext.QuotationsView
        .Where(q => filter.StoreId == null || q.StoreId == filter.StoreId)
        .Where(q => filter.VendorId == null || q.VendorId == filter.VendorId)
        .Where(q => string.IsNullOrEmpty(filter.SearchText) ||
            EF.Functions.Like(q.ClientName, $"%{filter.SearchText}%"))
        .Where(q => filter.Semaphore == null || q.Semaphore == filter.Semaphore)
        .ToListAsync();

    return Ok(quotations);
}
```

**Validation Rules:**
- ✅ Data type validation
- ✅ Range validation (for numeric IDs and semáforo)
- ✅ String length limits
- ✅ Pattern matching (date format, search text)
- ✅ Null/empty handling
- ✅ Date parsing validation
- ✅ Parameterized LINQ queries (SQL injection prevention)

---

### Laravel

**Filter Validation:**
```php
// FilterRequest - Form Request Validation
namespace App\Http\Requests;

class FilterRequest extends FormRequest
{
    public function authorize()
    {
        return auth()->check();
    }

    public function rules()
    {
        return [
            'id_loja' => 'nullable|integer|exists:loja,id_loja',
            'id_vendedor' => 'nullable|integer|exists:usuario,id_usuario',
            'fornecedor' => 'nullable|string|max:100',
            'mes_ano' => 'nullable|date_format:Y-m',
            'search' => 'nullable|string|max:500|regex:/^[a-zA-Z0-9\s\-áéíóúãõç]*$/',
            'semaforo' => 'nullable|integer|in:1,2,3',
            'statuses' => 'nullable|array',
            'statuses.*' => 'in:ATIVO,FECHADO,EXPIRADO,PERDIDO,CANCELADO,REPLICADO',
        ];
    }

    public function messages()
    {
        return [
            'id_loja.exists' => 'Selected store does not exist',
            'id_vendedor.exists' => 'Selected vendor does not exist',
            'mes_ano.date_format' => 'Month must be in YYYY-MM format',
            'semaforo.in' => 'Invalid semaphore value',
            'statuses.in' => 'Invalid status value',
            'search.regex' => 'Search contains invalid characters',
        ];
    }
}

// QuotationController
public function filter(FilterRequest $request)
{
    // Request is already validated
    $query = OrcamentoView::query();

    // Apply filters with validation
    if ($request->filled('id_loja')) {
        $query->where('id_loja', $request->id_loja);
    }

    if ($request->filled('id_vendedor')) {
        $query->where(function($q) use ($request) {
            $q->where('id_usuario', $request->id_vendedor)
              ->orWhere('id_usuario_consultor', $request->id_vendedor);
        });
    }

    if ($request->filled('fornecedor')) {
        $query->whereRaw('FIND_IN_SET(?, fornecedores) > 0', [$request->fornecedor]);
    }

    if ($request->filled('mes_ano')) {
        $query->where('data2', $request->mes_ano);
    }

    if ($request->filled('search')) {
        $search = $request->search;
        $query->where(function($q) use ($search) {
            $q->where('id_orcamento', 'like', "%{$search}%")
              ->orWhere('vendedor', 'like', "%{$search}%")
              ->orWhere('cliente', 'like', "%{$search}%");
        });
    }

    if ($request->filled('semaforo')) {
        $query->where('semaforo', $request->semaforo);
    }

    if ($request->filled('statuses')) {
        $query->whereIn('status', $request->statuses);
    }

    return response()->json($query->paginate(50));
}
```

**Validation Rules:**
- ✅ Existence validation (id_loja, id_vendedor exist in database)
- ✅ Date format validation (Y-m format)
- ✅ Enum validation (specific allowed values for status, semáforo)
- ✅ String length limits
- ✅ Pattern matching (search text)
- ✅ Array validation (status checkboxes)
- ✅ Parameterized queries (SQL injection prevention)
- ✅ Custom error messages

---

### Symfony

**Filter Query Validation:**
```php
// OrcamentoController - Filter validation
public function getData(Request $request): JsonResponse
{
    $session = $request->getSession();
    if (!$session->has('usuario_id')) {
        return $this->json(['error' => 'Unauthorized'], Response::HTTP_UNAUTHORIZED);
    }

    // Get filters from request
    $filters = $request->query->all();

    // Validate filter inputs
    $errors = $this->validateFilters($filters);
    if (!empty($errors)) {
        return $this->json(['errors' => $errors], Response::HTTP_BAD_REQUEST);
    }

    try {
        // Get filtered data
        $orcamentos = $this->getFilteredOrcamentos($usuario, $filters);

        // Convert to array format
        $data = array_map(fn(OrcamentoView $o) => $o->toArray(), $orcamentos);

        return $this->json(['success' => true, 'data' => $data]);
    } catch (\Exception $e) {
        error_log('[ORCAMENTOS] Filter error: ' . $e->getMessage());
        return $this->json(['error' => 'Error loading quotations'],
            Response::HTTP_INTERNAL_SERVER_ERROR);
    }
}

private function validateFilters(array $filters): array
{
    $errors = [];

    // Validate idLoja if provided
    if (isset($filters['idLoja'])) {
        if (!is_numeric($filters['idLoja'])) {
            $errors['idLoja'] = 'Store ID must be numeric';
        } elseif ($filters['idLoja'] < 1) {
            $errors['idLoja'] = 'Invalid store ID';
        }
    }

    // Validate month format (YYYY-MM)
    if (!empty($filters['mesAno'])) {
        if (!preg_match('/^\d{4}-\d{2}$/', $filters['mesAno'])) {
            $errors['mesAno'] = 'Invalid month format (use YYYY-MM)';
        } else {
            // Validate actual date
            [$year, $month] = explode('-', $filters['mesAno']);
            if ($month < 1 || $month > 12) {
                $errors['mesAno'] = 'Invalid month (1-12)';
            }
        }
    }

    // Validate vendor ID
    if (!empty($filters['idVendedor'])) {
        if (!is_numeric($filters['idVendedor'])) {
            $errors['idVendedor'] = 'Vendor ID must be numeric';
        }
    }

    // Validate search text
    if (!empty($filters['search'])) {
        if (strlen($filters['search']) > 500) {
            $errors['search'] = 'Search text too long (max 500 characters)';
        }
        // Check for potentially malicious patterns
        if (preg_match('/[<>"\'];/', $filters['search'])) {
            $errors['search'] = 'Invalid characters in search';
        }
    }

    // Validate supplier
    if (!empty($filters['fornecedor'])) {
        if (strlen($filters['fornecedor']) > 200) {
            $errors['fornecedor'] = 'Supplier name too long';
        }
    }

    // Validate semáforo (1=QUENTE, 2=MORNO, 3=FRIO)
    if (isset($filters['semaforo']) && $filters['semaforo'] !== '') {
        if (!in_array((int)$filters['semaforo'], [1, 2, 3])) {
            $errors['semaforo'] = 'Invalid semaphore value';
        }
    }

    // Validate status array
    if (!empty($filters['statuses'])) {
        $validStatuses = ['ATIVO', 'FECHADO', 'EXPIRADO', 'PERDIDO', 'CANCELADO', 'REPLICADO'];
        if (is_array($filters['statuses'])) {
            foreach ($filters['statuses'] as $status) {
                if (!in_array($status, $validStatuses)) {
                    $errors['statuses'] = 'Invalid status value: ' . $status;
                    break;
                }
            }
        }
    }

    return $errors;
}

// In getFilteredOrcamentos()
private function getFilteredOrcamentos(Usuario $user, array $filters): array
{
    $qb = $this->entityManager->createQueryBuilder()
        ->select('o')
        ->from(OrcamentoView::class, 'o');

    // Role-based filtering with validation
    if ($user->isGerente() || $user->isAssistenteAdministrativo()) {
        $qb->where('o.idLoja = :idLoja')
            ->setParameter('idLoja', $user->getIdLoja());
    }

    // Additional filters with type-safe parameterization
    if (!empty($filters['mesAno'])) {
        $qb->andWhere('o.data2 = :mesAno')
            ->setParameter('mesAno', $filters['mesAno']);
    }

    if (!empty($filters['idVendedor'])) {
        $qb->andWhere('(o.idUsuario = :idVendedor OR o.idUsuarioConsultor = :idVendedor)')
            ->setParameter('idVendedor', (int)$filters['idVendedor']);
    }

    // FIND_IN_SET for CSV column
    if (!empty($filters['fornecedor'])) {
        $qb->andWhere("FIND_IN_SET(:fornecedor, o.fornecedores) > 0")
            ->setParameter('fornecedor', $filters['fornecedor']);
    }

    // Status array validation
    if (!empty($filters['statuses']) && is_array($filters['statuses'])) {
        $qb->andWhere('o.status IN (:statuses)')
            ->setParameter('statuses', $filters['statuses']);
    }

    // Semáforo enum validation
    if (isset($filters['semaforo']) && $filters['semaforo'] !== '') {
        $qb->andWhere('o.semaforo = :semaforo')
            ->setParameter('semaforo', (int)$filters['semaforo']);
    }

    // Text search with parameterized queries
    if (!empty($filters['search'])) {
        $search = '%' . $filters['search'] . '%';
        $qb->andWhere('(o.idOrcamento LIKE :search OR o.cliente LIKE :search OR o.vendedor LIKE :search)')
            ->setParameter('search', $search);
    }

    $qb->orderBy('o.data', 'DESC');
    $qb->setMaxResults(500);

    return $qb->getQuery()->getResult();
}
```

**Validation Rules:**
- ✅ Numeric validation for IDs
- ✅ Date format validation (YYYY-MM)
- ✅ Month range validation (1-12)
- ✅ String length limits
- ✅ Enum validation (status, semáforo)
- ✅ Array element validation
- ✅ Malicious pattern detection (<>, quotes, semicolons)
- ✅ Type casting for parameterized queries
- ✅ Role-based filtering
- ✅ Extensive logging for debugging

---

### TypeScript/Node.js

**Filter Validation with Zod:**
```typescript
import { z } from 'zod';

const quotationFilterSchema = z.object({
  idLoja: z.number().int().positive().optional(),
  idVendedor: z.number().int().positive().optional(),
  fornecedor: z.string().max(200).optional(),
  mesAno: z.string().regex(/^\d{4}-\d{2}$/, 'Invalid month format').optional(),
  search: z.string()
    .max(500, 'Search text too long')
    .regex(/^[a-zA-Z0-9\s\-áéíóúãõç]*$/, 'Invalid characters')
    .optional(),
  semaforo: z.enum(['1', '2', '3']).optional(),
  statuses: z.array(z.enum(['ATIVO', 'FECHADO', 'EXPIRADO', 'PERDIDO', 'CANCELADO', 'REPLICADO']))
    .optional(),
  page: z.number().int().positive().default(1),
  limit: z.number().int().min(10).max(500).default(50),
}).strict();

// Filter endpoint
quotationRouter.get('/filter', async (req: Request, res: Response) => {
  try {
    // Validate filters
    const filters = quotationFilterSchema.parse(req.query);

    // Build query with validation
    let query = 'SELECT * FROM view_orcamento WHERE 1=1';
    const params: any[] = [];

    // Store filter
    if (filters.idLoja) {
      query += ' AND id_loja = $' + (params.length + 1);
      params.push(filters.idLoja);
    }

    // Vendor filter
    if (filters.idVendedor) {
      query += ' AND (id_usuario = $' + (params.length + 1) +
               ' OR id_usuario_consultor = $' + (params.length + 1) + ')';
      params.push(filters.idVendedor);
    }

    // Month filter
    if (filters.mesAno) {
      query += ' AND data2 = $' + (params.length + 1);
      params.push(filters.mesAno);
    }

    // Search filter with parameterization
    if (filters.search) {
      query += ' AND (id_orcamento LIKE $' + (params.length + 1) +
               ' OR cliente LIKE $' + (params.length + 1) + ')';
      const searchTerm = `%${filters.search}%`;
      params.push(searchTerm);
    }

    // Semaphore filter
    if (filters.semaforo) {
      query += ' AND semaforo = $' + (params.length + 1);
      params.push(parseInt(filters.semaforo));
    }

    // Status filter
    if (filters.statuses && filters.statuses.length > 0) {
      const placeholders = filters.statuses.map((_, i) =>
        '$' + (params.length + i + 1)
      ).join(',');
      query += ` AND status IN (${placeholders})`;
      params.push(...filters.statuses);
    }

    // Pagination
    const offset = (filters.page - 1) * filters.limit;
    query += ` ORDER BY data DESC LIMIT $${params.length + 1} OFFSET $${params.length + 2}`;
    params.push(filters.limit, offset);

    // Execute query
    const result = await db.query(query, params);

    res.json({
      success: true,
      data: result.rows,
      pagination: {
        page: filters.page,
        limit: filters.limit,
        total: result.rowCount,
      },
    });
  } catch (error) {
    if (error instanceof z.ZodError) {
      return res.status(400).json({
        success: false,
        errors: error.flatten().fieldErrors,
      });
    }

    res.status(500).json({ success: false, message: 'Server error' });
  }
});
```

**Validation Rules:**
- ✅ Zod schema validation (TypeScript-first)
- ✅ Numeric validation with range
- ✅ Date format validation
- ✅ String length limits
- ✅ Pattern matching
- ✅ Enum validation (strict values)
- ✅ Array element validation
- ✅ Parameterized SQL queries
- ✅ Strict parsing (no extra fields)
- ✅ Default values
- ✅ Detailed error messages

---

## Database Level Validation

### All Implementations Use:

**Column Constraints:**
```sql
-- usuario table
CREATE TABLE usuario (
    idUsuario INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    user VARCHAR(45) NOT NULL UNIQUE,
    password VARCHAR(255) NOT NULL,
    nome VARCHAR(150) NOT NULL,
    tipo VARCHAR(45) NOT NULL CHECK (tipo IN ('ADMINISTRADOR', 'DIRETOR', ...)),
    idLoja INT NOT NULL,
    desativado BOOLEAN DEFAULT FALSE,
    FOREIGN KEY (idLoja) REFERENCES loja(idLoja)
);

-- Indices for performance
CREATE INDEX idx_usuario_user ON usuario(user);
CREATE INDEX idx_usuario_tipo ON usuario(tipo);
```

**Validation Hierarchy:**
1. **Client-side** (HTML5, JavaScript) - Quick feedback
2. **Server-side** (Business logic) - Security critical
3. **Database** (Constraints, triggers) - Final safety net

---

## Comparison Summary Table

| Validation Type | C++ Qt | .NET | Laravel | Symfony | TypeScript |
|---|---|---|---|---|---|
| **HTML5 Validation** | N/A | ✅ | ✅ | ✅ | ✅ |
| **Client JS** | ✅ Qt | ✅ | ⚠️ Optional | ⚠️ Optional | ✅ Zod |
| **Form Requests** | ✅ Qt | ✅ | ✅ | ✅ | ✅ |
| **Length Limits** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **Pattern Regex** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **Type Validation** | ✅ Qt types | ✅ | ⚠️ Loose | ⚠️ Loose | ✅ TypeScript |
| **Existence Check** | ✅ SQL | ✅ | ✅ | ✅ | ✅ |
| **Enum Validation** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **Date Format** | ✅ QDate | ✅ | ✅ | ✅ | ✅ |
| **SQL Injection Prevention** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **CSRF Protection** | N/A | ✅ | ✅ | ✅ | ✅ |
| **Rate Limiting** | N/A | ⚠️ | ⚠️ | ❌ | ✅ |
| **Error Logging** | ✅ | ✅ | ✅ | ✅ | ✅ |

---

## Best Practices Across All Implementations

### 1. Never Trust Client Input
```
✅ Always validate on server
✅ Use parameterized queries
✅ Check data types and ranges
❌ Never rely solely on HTML5 validation
```

### 2. Clear Error Messages
```
✅ User-friendly messages for clients
✅ Detailed logging for developers
✅ Don't expose system details
❌ Generic "Server Error" messages
```

### 3. Consistent Validation
```
✅ Same rules on client and server
✅ Database constraints as final layer
✅ Centralized validation logic
❌ Different rules in different places
```

### 4. Performance
```
✅ Validate early (fail fast)
✅ Cache validation results where appropriate
✅ Avoid redundant checks
❌ Validate in loops
```

### 5. Security
```
✅ Parameterized queries everywhere
✅ Input length limits
✅ Pattern matching for formats
✅ Rate limiting for login attempts
❌ String concatenation for SQL
❌ Accepting unlimited input
```

---

## Recommended Validation Stack

### For Maximum Security:
1. **HTML5 Validation** (immediate feedback)
2. **JavaScript Validation** (enhanced UX)
3. **Server-side Validation** (security critical)
4. **Database Constraints** (final safety net)
5. **Parameterized Queries** (SQL injection prevention)

### Minimum Required:
1. **Server-side Validation** (non-negotiable)
2. **Database Constraints** (schema integrity)
3. **Parameterized Queries** (SQL injection prevention)

---

## Conclusion

All five implementations validate data thoroughly using similar patterns:

- **C++ Qt:** Qt framework handles types; custom validators for UI
- **.NET:** DataAnnotations + model validation framework
- **Laravel:** Form Request classes with fluent rules
- **Symfony:** Manual validation + extensive logging (security-first)
- **TypeScript:** Zod schema validation (TypeScript-native)

**Best approach:** Layer validations at multiple levels with parameterized queries as the foundation.
