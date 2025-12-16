# Validation Libraries Comparison Guide - 2025

Comprehensive comparison of the top validation libraries for each language/framework used in the Staccato ERP project.

---

## Table of Contents

1. [C++ (Qt 5.15)](#c-qt-515)
2. [.NET 8](#net-8)
3. [PHP (Laravel & Symfony)](#php-laravel--symfony)
4. [TypeScript/JavaScript](#typescriptjavascript)
5. [Cross-Language Comparison](#cross-language-comparison)
6. [Recommendations for Staccato ERP](#recommendations-for-staccato-erp)

---

## C++ (Qt 5.15)

### Top 3 Libraries

#### 1. Built-in QValidator Classes (Qt Framework)

**Overview**: Qt provides native validator classes for form input validation in Qt Widgets.

**Pros**:
- ✅ Zero external dependencies
- ✅ Seamless Qt integration with QLineEdit, QSpinBox, QComboBox
- ✅ Excellent performance (compiled into Qt)
- ✅ Three-state validation: Invalid, Intermediate, Acceptable
- ✅ Built-in regex support (QRegularExpressionValidator)

**Cons**:
- ❌ Limited to simple validation rules
- ❌ No async validation support
- ❌ Requires subclassing for complex logic
- ❌ View-layer only (no business logic validation)

**Best For**: Simple form field validation (email, numbers, patterns)

**Code Example - Login Form Validation**:
```cpp
#include <QLineEdit>
#include <QRegularExpressionValidator>
#include <QIntValidator>

class LoginDialog : public QDialog {
private:
    QLineEdit* userInput;
    QLineEdit* passwordInput;

public:
    LoginDialog() {
        userInput = new QLineEdit();

        // Email/username validation: only alphanumeric and underscore
        QRegularExpression userRegex("^[a-zA-Z0-9_]{3,20}$");
        QRegularExpressionValidator* userValidator =
            new QRegularExpressionValidator(userRegex, userInput);
        userInput->setValidator(userValidator);

        // Password must be at least 4 characters
        // (though Qt doesn't hide content for validation purposes)
        passwordInput = new QLineEdit();

        // Custom validator for complex password rules
        PasswordValidator* pwdValidator = new PasswordValidator(passwordInput);
        passwordInput->setValidator(pwdValidator);
    }
};

// Custom validator for complex rules
class PasswordValidator : public QValidator {
public:
    State validate(QString &input, int &pos) const override {
        // Password: 4+ chars, must contain number or special char
        if (input.isEmpty()) return Intermediate;
        if (input.length() < 4) return Intermediate;

        bool hasNumber = input.contains(QRegularExpression("\\d"));
        bool hasSpecial = input.contains(QRegularExpression("[!@#$%^&*]"));

        if (input.length() >= 4 && (hasNumber || hasSpecial)) {
            return Acceptable;
        }
        return Intermediate;
    }
};
```

**Quotations Filter Validation**:
```cpp
// Vendor dropdown - only numeric IDs
QComboBox* vendorDropdown = new QComboBox();
vendorDropdown->addItem("Todos", 0);
// Validator ensures only numbers in hidden ID field

// Date field validation - DD/MM/YYYY format
QLineEdit* dateField = new QLineEdit();
QRegularExpression dateRegex("^(0[1-9]|[12][0-9]|3[01])/(0[1-9]|1[0-2])/\\d{4}$");
QRegularExpressionValidator* dateValidator =
    new QRegularExpressionValidator(dateRegex, dateField);
dateField->setValidator(dateValidator);

// Status checkboxes - no validation needed (binary state)
// Semáforo radio buttons - no validation needed (predefined values)
```

---

#### 2. cpp-validator (Header-Only Generic Validation)

**Overview**: Modern C++ header-only library for generic data validation with automatic error messages.

**Pros**:
- ✅ Header-only (easy integration, no compilation)
- ✅ Declarative syntax - very readable
- ✅ Automatic error message generation with i18n support
- ✅ Supports objects, containers, variables
- ✅ C++14/C++17 compatible
- ✅ Zero runtime overhead for unused features
- ✅ Custom validation rules

**Cons**:
- ❌ Not Qt-integrated (need manual binding)
- ❌ No async validation
- ❌ Smaller community than Qt validators
- ❌ Requires modern C++ compiler

**Best For**: Business logic validation layer (model layer)

**Code Example - Quotation Filter Validation**:
```cpp
#include <cpp-validator/Validator.hpp>
#include <cpp-validator/Rule.hpp>

using namespace validator;

struct QuotationFilter {
    std::optional<int> vendorId;
    std::optional<std::string> monthYear;  // Format: YYYY-MM
    std::optional<std::string> supplier;
    std::vector<std::string> statuses;     // ["ativo", "cancelado", ...]
    std::optional<int> semaforo;           // 0=todos, 1=quente, 2=morno, 3=frio
    std::optional<std::string> searchText;
};

// Define validation rules
auto validateQuotationFilter = [](const QuotationFilter& filter) {
    auto validator = Validator<QuotationFilter>()
        // Vendor ID: positive integer if provided
        .add(Rule(&QuotationFilter::vendorId)
            .when_set()
            .greater_than(0)
            .error_message("Vendor ID must be positive"))

        // Month/Year: YYYY-MM format if provided
        .add(Rule(&QuotationFilter::monthYear)
            .when_set()
            .matches("^\\d{4}-(0[1-9]|1[0-2])$")
            .error_message("Month format must be YYYY-MM"))

        // Semáforo: 0 (todos), 1 (quente), 2 (morno), 3 (frio)
        .add(Rule(&QuotationFilter::semaforo)
            .when_set()
            .in({0, 1, 2, 3})
            .error_message("Invalid semáforo value"))

        // Statuses: must be valid status values
        .add(Rule(&QuotationFilter::statuses)
            .when_not_empty()
            .each()
                .in({"ativo", "cancelado", "expirado", "fechado", "perdido", "replicado"})
                .error_message("Invalid status value"));

    return validator.validate(filter);
};

// Usage in controller
QuotationFilter filter;
filter.vendorId = 5;
filter.monthYear = "2025-12";
filter.statuses = {"ativo", "fechado"};

auto result = validateQuotationFilter(filter);
if (!result.is_valid()) {
    for (const auto& error : result.errors()) {
        std::cerr << error.field() << ": " << error.message() << std::endl;
    }
}
```

---

#### 3. Valijson (JSON Schema Validation)

**Overview**: Header-only C++ library for JSON Schema validation (Draft 3, 4, 7).

**Pros**:
- ✅ Header-only
- ✅ Multiple JSON parser support (RapidJSON, jsoncpp, nlohmann/json)
- ✅ Standards-compliant (JSON Schema)
- ✅ Excellent for API response validation
- ✅ Schema reusability across languages

**Cons**:
- ❌ JSON Schema has learning curve
- ❌ No async validation
- ❌ Verbose for simple validations
- ❌ Best for JSON documents, not C++ objects directly

**Best For**: Validating JSON responses from backend APIs

**Code Example - API Response Validation**:
```cpp
#include <valijson/valijson.hpp>
#include <valijson/validators/null_validator.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Define JSON Schema for quotation list API response
json quotationListSchema = R"({
    "type": "object",
    "required": ["success", "data"],
    "properties": {
        "success": {
            "type": "boolean"
        },
        "data": {
            "type": "array",
            "items": {
                "type": "object",
                "required": ["idOrcamento", "status", "total", "data"],
                "properties": {
                    "idOrcamento": { "type": "string" },
                    "status": {
                        "type": "string",
                        "enum": ["ativo", "cancelado", "expirado", "fechado", "perdido", "replicado"]
                    },
                    "total": {
                        "type": "number",
                        "minimum": 0
                    },
                    "data": {
                        "type": "string",
                        "pattern": "^\\d{2}/\\d{2}/\\d{4}$"  // DD/MM/YYYY
                    },
                    "diasRestantes": {
                        "type": "integer",
                        "minimum": 0
                    }
                }
            }
        }
    }
})"_json;

// Validate API response
void validateQuotationResponse(const std::string& jsonString) {
    json document = json::parse(jsonString);

    valijson::Schema schema;
    valijson::SchemaParser parser;
    valijson::JsonSchemaValidator validator(schema);

    parser.populateSchema(quotationListSchema, schema);

    if (!validator.validate(document)) {
        std::cerr << "Validation failed:" << std::endl;
        for (const auto& error : validator.errors()) {
            std::cerr << error.message() << std::endl;
        }
    }
}
```

---

### Summary - C++ (Qt 5.15)

| Feature | QValidator | cpp-validator | Valijson |
|---------|-----------|----------------|----------|
| **Setup** | Built-in | Header-only | Header-only |
| **Best Use** | UI field validation | Business logic | API response validation |
| **Learning Curve** | Low | Medium | Medium-High |
| **Performance** | Excellent | Excellent | Excellent |
| **Async Support** | No | No | No |
| **Custom Rules** | Via subclassing | Via lambda | Via schema |
| **Error Messages** | Basic | Auto-generated | Schema-based |
| **Integration with Qt** | Seamless | Manual | Manual |

**Recommendation for Staccato ERP**:
- **UI Layer**: QValidator subclasses (built-in)
- **Model Layer**: cpp-validator (for business logic)
- **API Layer**: Valijson (for response validation)

---

## .NET 8

### Top 3 Libraries

#### 1. FluentValidation (Industry Standard)

**Overview**: The most popular .NET validation library with fluent API, extensive customization, and enterprise features.

**Stats**:
- Downloads: **783.7M total** (143.7K/day)
- GitHub Stars: 8,000+
- Current Version: 12.1.1

**Pros**:
- ✅ Fluent, chainable API (very readable)
- ✅ Async validation support (database queries, API calls)
- ✅ Complex conditional validation
- ✅ Rule sets for different scenarios
- ✅ Custom validators easily extensible
- ✅ Detailed error messages with property paths
- ✅ ASP.NET Core middleware integration
- ✅ Excellent test support
- ✅ Dependency injection support
- ✅ MediatR pipeline integration

**Cons**:
- ❌ Slightly slower than DataAnnotations for simple cases
- ❌ More code for basic validations
- ❌ Not built-in (external dependency)

**Best For**: Enterprise applications, complex validation rules, production systems

**Code Example - Login Validation**:
```csharp
using FluentValidation;
using FluentValidation.Results;

public class LoginRequest
{
    public string Username { get; set; }
    public string Password { get; set; }
    public bool RememberMe { get; set; }
}

public class LoginValidator : AbstractValidator<LoginRequest>
{
    private readonly IUsuarioRepository _usuarioRepository;

    public LoginValidator(IUsuarioRepository usuarioRepository)
    {
        _usuarioRepository = usuarioRepository;

        // Username validation
        RuleFor(x => x.Username)
            .NotEmpty()
                .WithMessage("Username is required")
            .Length(3, 50)
                .WithMessage("Username must be between 3 and 50 characters")
            .Matches(@"^[a-zA-Z0-9_]+$")
                .WithMessage("Username can only contain letters, numbers, and underscores")
            .MustAsync(UserMustExist, "User does not exist");

        // Password validation
        RuleFor(x => x.Password)
            .NotEmpty()
                .WithMessage("Password is required")
            .Length(4, 100)
                .WithMessage("Password must be between 4 and 100 characters");
    }

    // Custom async rule for database lookup
    private async Task<bool> UserMustExist(string username, CancellationToken cancellation)
    {
        // Check if user exists and is not OPERACIONAL type
        var user = await _usuarioRepository.FindByUsernameAsync(username);
        return user != null && user.Tipo != UserType.OPERACIONAL;
    }
}

// Usage in controller
public class AuthController : ControllerBase
{
    private readonly IValidator<LoginRequest> _validator;

    [HttpPost("login")]
    public async Task<IActionResult> Login([FromBody] LoginRequest request)
    {
        // Validate request
        var result = await _validator.ValidateAsync(request);

        if (!result.IsValid)
        {
            return BadRequest(new { errors = result.Errors });
        }

        // Proceed with authentication
        // ...
    }
}
```

**Code Example - Quotation Filter Validation**:
```csharp
public class QuotationFilterRequest
{
    public int? VendorId { get; set; }
    public string MonthYear { get; set; }  // YYYY-MM format
    public string Supplier { get; set; }
    public List<string> Statuses { get; set; }
    public int? Semaforo { get; set; }     // 0=todos, 1=quente, 2=morno, 3=frio
    public string SearchText { get; set; }
}

public class QuotationFilterValidator : AbstractValidator<QuotationFilterRequest>
{
    public QuotationFilterValidator()
    {
        // Vendor ID: positive integer if provided
        RuleFor(x => x.VendorId)
            .GreaterThan(0)
                .When(x => x.VendorId.HasValue)
                .WithMessage("Vendor ID must be positive");

        // Month/Year: YYYY-MM format
        RuleFor(x => x.MonthYear)
            .Matches(@"^\d{4}-(0[1-9]|1[0-2])$")
                .When(x => !string.IsNullOrEmpty(x.MonthYear))
                .WithMessage("Month format must be YYYY-MM");

        // Semáforo: only valid values
        RuleFor(x => x.Semaforo)
            .Must(s => new[] { 0, 1, 2, 3 }.Contains(s.Value))
                .When(x => x.Semaforo.HasValue)
                .WithMessage("Semáforo must be 0 (todos), 1 (quente), 2 (morno), or 3 (frio)");

        // Statuses: must be valid status values
        RuleFor(x => x.Statuses)
            .NotNull()
            .Must(list => list.All(status =>
                new[] { "ativo", "cancelado", "expirado", "fechado", "perdido", "replicado" }
                .Contains(status)))
            .WithMessage("Contains invalid status value");

        // Search text: max length
        RuleFor(x => x.SearchText)
            .MaximumLength(100)
                .When(x => !string.IsNullOrEmpty(x.SearchText))
                .WithMessage("Search text cannot exceed 100 characters");
    }
}
```

---

#### 2. DataAnnotations (Built-in - Lightweight)

**Overview**: .NET's native validation system using attributes, with zero external dependencies.

**Pros**:
- ✅ Built-in to .NET framework
- ✅ Zero external dependencies
- ✅ Excellent performance
- ✅ Seamless ASP.NET MVC integration
- ✅ Client-side validation support
- ✅ Simple attribute-based syntax

**Cons**:
- ❌ Limited to simple validations
- ❌ No async support (until recently, complex to implement)
- ❌ Conditional validation is awkward
- ❌ Less flexible than FluentValidation
- ❌ Less detailed error messages by default

**Best For**: Simple CRUD applications, rapid prototyping, minimal dependencies

**Code Example - Login Validation**:
```csharp
using System.ComponentModel.DataAnnotations;

public class LoginRequest
{
    [Required(ErrorMessage = "Username is required")]
    [StringLength(50, MinimumLength = 3,
        ErrorMessage = "Username must be between 3 and 50 characters")]
    [RegularExpression(@"^[a-zA-Z0-9_]+$",
        ErrorMessage = "Username can only contain letters, numbers, and underscores")]
    public string Username { get; set; }

    [Required(ErrorMessage = "Password is required")]
    [StringLength(100, MinimumLength = 4,
        ErrorMessage = "Password must be between 4 and 100 characters")]
    public string Password { get; set; }

    public bool RememberMe { get; set; }
}

// Validation in controller
[HttpPost("login")]
public IActionResult Login([FromBody] LoginRequest request)
{
    if (!ModelState.IsValid)
    {
        return BadRequest(ModelState);
    }

    // Proceed with authentication
    // ...
}
```

**Code Example - Quotation Filter Validation**:
```csharp
public class QuotationFilterRequest
{
    [Range(1, int.MaxValue, ErrorMessage = "Vendor ID must be positive")]
    public int? VendorId { get; set; }

    [RegularExpression(@"^\d{4}-(0[1-9]|1[0-2])$",
        ErrorMessage = "Month format must be YYYY-MM")]
    public string MonthYear { get; set; }

    [StringLength(100)]
    public string Supplier { get; set; }

    [ValidStatuses]
    public List<string> Statuses { get; set; }

    [Range(0, 3, ErrorMessage = "Semáforo must be 0-3")]
    public int? Semaforo { get; set; }

    [StringLength(100)]
    public string SearchText { get; set; }
}

// Custom attribute for status validation
[AttributeUsage(AttributeTargets.Property)]
public class ValidStatusesAttribute : ValidationAttribute
{
    private static readonly string[] ValidStatuses =
        { "ativo", "cancelado", "expirado", "fechado", "perdido", "replicado" };

    protected override ValidationResult IsValid(object value, ValidationContext context)
    {
        var statuses = value as List<string>;
        if (statuses == null) return ValidationResult.Success;

        if (!statuses.All(s => ValidStatuses.Contains(s)))
        {
            return new ValidationResult("Contains invalid status value");
        }

        return ValidationResult.Success;
    }
}
```

---

#### 3. Validot (Performance Champion)

**Overview**: .NET validation library optimized for speed and memory efficiency, 13.3x better than FluentValidation.

**Stats**:
- Performance: ~13.3x less memory than FluentValidation
- Compilation: Generates optimized validation code at compile time
- Adoption: Growing in high-performance scenarios

**Pros**:
- ✅ Exceptional performance (13.3x better memory than FluentValidation)
- ✅ Compile-time validation plan generation
- ✅ Fluent API
- ✅ Custom validators
- ✅ Translation support
- ✅ DI container integration
- ✅ Suitable for high-throughput APIs

**Cons**:
- ❌ Less mature than FluentValidation
- ❌ Smaller community
- ❌ No async validation (by design)
- ❌ More limited feature set

**Best For**: High-performance APIs, microservices, large-scale systems

**Code Example - Quotation Filter Validation**:
```csharp
using Validot;
using Validot.Specification;

public class QuotationFilterValidator : Specification<QuotationFilterRequest>
{
    public QuotationFilterValidator()
    {
        // Vendor ID validation
        RuleFor(x => x.VendorId)
            .GreaterThan(0)
            .Optional();

        // Month/Year validation
        RuleFor(x => x.MonthYear)
            .Matches(@"^\d{4}-(0[1-9]|1[0-2])$")
            .Optional();

        // Semáforo validation
        RuleFor(x => x.Semaforo)
            .Must(s => new[] { 0, 1, 2, 3 }.Contains(s))
            .Optional();

        // Statuses validation
        RuleFor(x => x.Statuses)
            .ForEach(status => status
                .Must(s => new[] { "ativo", "cancelado", "expirado", "fechado", "perdido", "replicado" }
                    .Contains(s)))
            .Optional();

        // Search text validation
        RuleFor(x => x.SearchText)
            .MaxLength(100)
            .Optional();
    }
}

// Usage
var specification = new QuotationFilterValidator();
var validator = specification.Build();

var result = validator.Validate(filter);
if (!result.IsValid)
{
    var errors = result.Errors;  // Dictionary of field -> error messages
}
```

---

### Summary - .NET 8

| Feature | FluentValidation | DataAnnotations | Validot |
|---------|-----------------|-----------------|---------|
| **Setup** | NuGet package | Built-in | NuGet package |
| **Downloads** | 783.7M | Built-in | Growing |
| **Complexity** | Medium | Low | Medium |
| **Performance** | Good | Excellent | **13.3x better** |
| **Memory** | Baseline | Lower | **Excellent** |
| **Async Support** | ✅ Yes | ❌ No | ❌ No |
| **Conditional Logic** | ✅ Easy | ❌ Difficult | ✅ Easy |
| **Learning Curve** | Medium | Low | Medium |

**Recommendation for Staccato ERP**:
- **Standard APIs**: FluentValidation (best for developer experience)
- **High-throughput Quotation Queries**: Validot (for performance)
- **Simple MVCs**: DataAnnotations (minimum overhead)

---

## PHP (Laravel & Symfony)

### Top 3 Libraries

#### 1. Laravel Validation (Built-in Framework)

**Overview**: Laravel's native validation system with 90+ built-in rules, incredibly flexible.

**Pros**:
- ✅ Built into Laravel framework
- ✅ 90+ pre-built validation rules
- ✅ Fluent and array syntax options
- ✅ Form Request classes for organized validation
- ✅ Conditional validation (sometimes, required_if, etc.)
- ✅ Custom validation rules easily
- ✅ After hooks for post-validation logic
- ✅ Excellent error messages
- ✅ Laravel 12+: secureValidate() for stricter defaults
- ✅ Database-aware rules (unique, exists)

**Cons**:
- ❌ Laravel-specific (not portable)
- ❌ Can be verbose for very simple validations
- ❌ Conditional logic can get complex

**Best For**: Laravel applications, form validation, API request validation

**Code Example - Login Validation**:
```php
// app/Http/Requests/LoginRequest.php
namespace App\Http\Requests;

use Illuminate\Foundation\Http\FormRequest;

class LoginRequest extends FormRequest
{
    public function authorize(): bool
    {
        return true;
    }

    public function rules(): array
    {
        return [
            'user' => [
                'required',
                'string',
                'min:3',
                'max:50',
                'regex:/^[a-zA-Z0-9_]+$/',
                function ($attribute, $value, $fail) {
                    // Custom async validation: check if user exists
                    $usuario = \App\Models\Usuario::where('user', $value)->first();
                    if (!$usuario) {
                        $fail('User does not exist');
                    } elseif ($usuario->tipo === 'OPERACIONAL') {
                        $fail('This user type cannot log in');
                    }
                }
            ],
            'password' => [
                'required',
                'string',
                'min:4',
                'max:100',
            ]
        ];
    }

    public function messages(): array
    {
        return [
            'user.required' => 'Username is required',
            'user.min' => 'Username must be at least 3 characters',
            'user.regex' => 'Username can only contain letters, numbers, and underscores',
            'password.required' => 'Password is required',
            'password.min' => 'Password must be at least 4 characters',
        ];
    }
}

// In controller
namespace App\Http\Controllers;

class AuthController extends Controller
{
    public function login(LoginRequest $request)  // Validation happens automatically
    {
        // Request data is validated at this point
        $credentials = $request->validated();

        // Proceed with authentication
    }
}
```

**Code Example - Quotation Filter Validation**:
```php
// app/Http/Requests/FilterQuotationsRequest.php
namespace App\Http\Requests;

use Illuminate\Foundation\Http\FormRequest;
use Illuminate\Validation\Rule;

class FilterQuotationsRequest extends FormRequest
{
    public function authorize(): bool
    {
        return true;
    }

    public function rules(): array
    {
        return [
            'idVendedor' => [
                'nullable',
                'integer',
                'min:1',
                Rule::exists('usuarios', 'idUsuario')
                    ->where('tipo', ['VENDEDOR', 'VENDEDOR ESPECIAL'])
            ],
            'mesAno' => [
                'nullable',
                'date_format:Y-m',
                'before_or_equal:now',
            ],
            'fornecedor' => [
                'nullable',
                'string',
                'max:100',
            ],
            'statuses' => [
                'nullable',
                'array',
            ],
            'statuses.*' => [
                Rule::in(['ativo', 'cancelado', 'expirado', 'fechado', 'perdido', 'replicado'])
            ],
            'semaforo' => [
                'nullable',
                'integer',
                Rule::in([0, 1, 2, 3])  // 0=todos, 1=quente, 2=morno, 3=frio
            ],
            'search' => [
                'nullable',
                'string',
                'max:100',
            ],
        ];
    }

    public function messages(): array
    {
        return [
            'idVendedor.exists' => 'Selected vendor does not exist',
            'mesAno.date_format' => 'Month must be in YYYY-MM format',
            'statuses.*.in' => 'Invalid status value provided',
            'semaforo.in' => 'Semáforo must be 0, 1, 2, or 3',
        ];
    }
}

// In controller
public function filterQuotations(FilterQuotationsRequest $request)
{
    $filters = $request->validated();

    // Fetch filtered quotations from view_orcamento
    $quotations = $this->getFilteredQuotations($filters);

    return response()->json([
        'success' => true,
        'data' => $quotations
    ]);
}
```

---

#### 2. Symfony Validator (Framework Component)

**Overview**: Symfony's validation component following JSR-303 specification, works standalone or integrated.

**Pros**:
- ✅ Framework-integrated (Symfony full-stack) or standalone
- ✅ Multiple configuration formats (Annotations, XML, YAML, PHP)
- ✅ JSR-303 Bean Validation spec compliance
- ✅ 30+ built-in constraints
- ✅ Validation groups for different scenarios
- ✅ Custom constraints
- ✅ Object graph validation
- ✅ Excellent documentation
- ✅ Doctrine ORM integration

**Cons**:
- ❌ More verbose than Laravel for simple cases
- ❌ Steeper learning curve
- ❌ Configuration overhead

**Best For**: Symfony applications, enterprise projects, complex validation

**Code Example - Login Validation**:
```php
// src/Entity/Usuario.php
namespace App\Entity;

use Symfony\Component\Validator\Constraints as Assert;

class Usuario
{
    #[Assert\NotBlank]
    #[Assert\Length(min: 3, max: 50)]
    #[Assert\Regex(pattern: '/^[a-zA-Z0-9_]+$/', message: 'Username can only contain alphanumeric and underscore')]
    private string $user;

    #[Assert\NotBlank]
    #[Assert\Length(min: 4, max: 100)]
    private string $password;

    #[Assert\NotBlank]
    #[Assert\Choice(choices: ['ADMINISTRADOR', 'DIRETOR', 'GERENTE LOJA', 'VENDEDOR', 'VENDEDOR ESPECIAL', 'OPERACIONAL'])]
    private string $tipo;

    #[Assert\NotNull]
    #[Assert\Choice(choices: [false])]  // Not desativado
    private bool $desativado = false;
}

// src/Controller/AuthController.php
namespace App\Controller;

use Symfony\Component\Validator\ValidatorInterface;

class AuthController
{
    public function login(Request $request, ValidatorInterface $validator): Response
    {
        $usuario = new Usuario();
        $usuario->setUser($request->request->get('user'));
        $usuario->setPassword($request->request->get('password'));

        // Validate
        $errors = $validator->validate($usuario);

        if (count($errors) > 0) {
            return $this->render('auth/login.html.twig', [
                'errors' => $errors
            ]);
        }

        // Proceed with authentication
    }
}
```

**Code Example - Quotation Filter Validation**:
```php
// src/DTO/QuotationFilterDTO.php
namespace App\DTO;

use Symfony\Component\Validator\Constraints as Assert;

class QuotationFilterDTO
{
    #[Assert\Type('int')]
    #[Assert\GreaterThan(0)]
    private ?int $vendorId = null;

    #[Assert\Regex(pattern: '/^\d{4}-(0[1-9]|1[0-2])$/')]
    private ?string $monthYear = null;

    #[Assert\Type('string')]
    #[Assert\Length(max: 100)]
    private ?string $supplier = null;

    #[Assert\All(
        new Assert\Choice(choices: ['ativo', 'cancelado', 'expirado', 'fechado', 'perdido', 'replicado'])
    )]
    private array $statuses = [];

    #[Assert\Choice(choices: [0, 1, 2, 3])]
    private ?int $semaforo = null;

    #[Assert\Type('string')]
    #[Assert\Length(max: 100)]
    private ?string $searchText = null;

    // Getters...
}

// In controller
public function filterQuotations(Request $request, ValidatorInterface $validator): JsonResponse
{
    $filter = new QuotationFilterDTO();
    // ... populate from request

    $errors = $validator->validate($filter);
    if (count($errors) > 0) {
        return $this->json(['success' => false, 'errors' => $errors]);
    }

    // Proceed with filtering
}
```

---

#### 3. Respect/Validation (Most Powerful Standalone)

**Overview**: Framework-agnostic validation library with 150+ rules and chainable fluent API.

**Stats**:
- Downloads: **34.2M total**
- GitHub Stars: 5,000+
- Rules: 150+ fully tested

**Pros**:
- ✅ Framework-agnostic (works anywhere)
- ✅ 150+ built-in validation rules
- ✅ Chainable fluent API
- ✅ Natural language syntax
- ✅ Exception-based error handling
- ✅ Composite validations
- ✅ Custom validation rules
- ✅ No dependencies
- ✅ Has CPF/CNPJ validators (Brazilian compliance!)

**Cons**:
- ❌ Exception-based error handling (different pattern)
- ❌ Overkill for simple cases
- ❌ Steeper learning curve than Laravel/Symfony
- ❌ Requires manual error handling

**Best For**: Complex validations, Brazilian-specific rules (CPF/CNPJ), portable code

**Code Example - Login Validation**:
```php
use Respect\Validation\Validator as v;
use Respect\Validation\Exceptions\ValidationException;

class LoginValidator
{
    public function validate(array $data): array
    {
        try {
            v::stringType()
                ->length(3, 50)
                ->alnum('_')
                ->assert($data['user'] ?? null);

            v::stringType()
                ->length(4, 100)
                ->assert($data['password'] ?? null);

            // Check user exists (custom rule)
            $user = Usuario::where('user', $data['user'])->first();
            v::notEmpty()
                ->assert($user);

            v::notOptional()
                ->notEqual('OPERACIONAL')
                ->assert($user->tipo);

            return ['valid' => true];
        } catch (ValidationException $e) {
            return [
                'valid' => false,
                'errors' => $e->getMessages()
            ];
        }
    }
}

// Usage
$validator = new LoginValidator();
$result = $validator->validate($_POST);
if (!$result['valid']) {
    // Handle errors
}
```

**Code Example - Quotation Filter with CPF Validation**:
```php
use Respect\Validation\Validator as v;
use Respect\Validation\Exceptions\ValidationException;

class QuotationFilterValidator
{
    public function validateFilters(array $filters): array
    {
        try {
            // Vendor ID
            if (isset($filters['idVendedor'])) {
                v::integer()
                    ->positive()
                    ->assert($filters['idVendedor']);
            }

            // Month/Year (YYYY-MM format)
            if (isset($filters['mesAno'])) {
                v::regex('/^\d{4}-(0[1-9]|1[0-2])$/')
                    ->assert($filters['mesAno']);
            }

            // Statuses (array of valid values)
            if (isset($filters['statuses'])) {
                v::arrayType()
                    ->each(
                        v::in(['ativo', 'cancelado', 'expirado', 'fechado', 'perdido', 'replicado'])
                    )
                    ->assert($filters['statuses']);
            }

            // Semáforo (0-3)
            if (isset($filters['semaforo'])) {
                v::integer()
                    ->between(0, 3)
                    ->assert($filters['semaforo']);
            }

            // For Brazilian commerce, could validate supplier CPF/CNPJ
            if (isset($filters['supplier']) && $this->isCpfOrCnpj($filters['supplier'])) {
                v::anyOf(
                    v::cpf(),
                    v::cnpj()
                )->assert($filters['supplier']);
            }

            return ['valid' => true];
        } catch (ValidationException $e) {
            return [
                'valid' => false,
                'errors' => $e->getMessages()
            ];
        }
    }

    private function isCpfOrCnpj(string $value): bool
    {
        return preg_match('/^[\d.-]+$/', $value) && strlen(preg_replace('/\D/', '', $value)) >= 11;
    }
}
```

---

### Summary - PHP

| Feature | Laravel Validation | Symfony Validator | Respect/Validation |
|---------|-------------------|------------------|-------------------|
| **Setup** | Built-in | Built-in | Composer package |
| **Downloads** | Built-in | Millions | 34.2M |
| **Rules** | 90+ | 30+ | **150+** |
| **Async Support** | ✅ (Closures) | ❌ | ❌ |
| **Fluent API** | ✅ | ❌ (Config-based) | ✅ |
| **Error Handling** | Array-based | Constraint objects | Exception-based |
| **CPF/CNPJ Support** | ❌ | ❌ | **✅** |
| **Framework Coupling** | Tight | Integrated | None |

**Recommendation for Staccato ERP**:
- **Laravel Apps**: Built-in Laravel Validation (100% coverage)
- **Symfony Apps**: Symfony Validator with annotations
- **Complex/Brazilian Rules**: Respect/Validation for CPF/CNPJ validators

---

## TypeScript/JavaScript

### Top 3 Libraries

#### 1. Zod (Market Leader)

**Overview**: TypeScript-first schema validation with excellent type inference, perfect for end-to-end type safety.

**Stats**:
- Downloads: **64.5M weekly downloads** (highest in category)
- GitHub Stars: **39,908 stars**
- Bundle Size: ~50KB minified
- Performance: ~2M ops/sec

**Pros**:
- ✅ TypeScript-first design with perfect inference
- ✅ Schema composition and transformation
- ✅ Async validation support
- ✅ Parse, safeParse, and transform methods
- ✅ Refinements for custom logic
- ✅ Rich error formatting
- ✅ Coercion and preprocessing
- ✅ Standard Schema spec compliant
- ✅ Excellent integration: React Hook Form, tRPC, Next.js, Fastify
- ✅ Growing ecosystem

**Cons**:
- ❌ Slower than AJV for pure performance (~2M vs ~14M ops/sec)
- ❌ Moderate bundle size
- ❌ Learning curve steeper than simple validators

**Best For**: Modern TypeScript applications, full-stack development, type safety

**Code Example - Login Validation**:
```typescript
import { z } from 'zod';

// Define login schema
const LoginSchema = z.object({
    user: z.string()
        .min(3, "Username must be at least 3 characters")
        .max(50, "Username cannot exceed 50 characters")
        .regex(/^[a-zA-Z0-9_]+$/, "Username can only contain letters, numbers, and underscores"),
    password: z.string()
        .min(4, "Password must be at least 4 characters")
        .max(100, "Password cannot exceed 100 characters"),
    rememberMe: z.boolean().optional().default(false)
});

type LoginInput = z.infer<typeof LoginSchema>;  // Excellent type inference!

// Usage with async validation
const validateLogin = async (data: unknown) => {
    try {
        // Step 1: Schema validation
        const validated = await LoginSchema.parseAsync(data);

        // Step 2: Custom async validation (check if user exists)
        const user = await db.usuario.findUnique({
            where: { user: validated.user }
        });

        if (!user) {
            throw new Error("User does not exist");
        }

        if (user.tipo === 'OPERACIONAL') {
            throw new Error("This user type cannot log in");
        }

        return { success: true, data: validated };
    } catch (error) {
        if (error instanceof z.ZodError) {
            return {
                success: false,
                errors: error.flatten()  // Structured error format
            };
        }
        return {
            success: false,
            message: error instanceof Error ? error.message : 'Login failed'
        };
    }
};

// In API handler
export async function POST(req: Request) {
    const result = await validateLogin(await req.json());

    if (!result.success) {
        return Response.json(result, { status: 400 });
    }

    // Proceed with authentication
}
```

**Code Example - Quotation Filter Validation**:
```typescript
import { z } from 'zod';

// Define quotation filter schema
const QuotationFilterSchema = z.object({
    idVendedor: z.number().int().positive().optional(),
    mesAno: z.string()
        .regex(/^\d{4}-(0[1-9]|1[0-2])$/, "Format must be YYYY-MM")
        .optional(),
    fornecedor: z.string().max(100).optional(),
    statuses: z.array(
        z.enum(['ativo', 'cancelado', 'expirado', 'fechado', 'perdido', 'replicado'])
    ).optional(),
    semaforo: z.enum(['0', '1', '2', '3']).optional(),  // 0=todos, 1=quente, 2=morno, 3=frio
    search: z.string().max(100).optional(),
}).strict();  // No extra properties allowed

type QuotationFilter = z.infer<typeof QuotationFilterSchema>;

// With async custom validation
const validateQuotationFilter = async (data: unknown) => {
    const result = QuotationFilterSchema.safeParse(data);

    if (!result.success) {
        return {
            valid: false,
            errors: result.error.flatten()
        };
    }

    const filters = result.data;

    // Custom async validation: vendor must exist
    if (filters.idVendedor) {
        const vendor = await db.usuario.findUnique({
            where: { idUsuario: filters.idVendedor }
        });

        if (!vendor) {
            return {
                valid: false,
                message: "Selected vendor does not exist"
            };
        }
    }

    return {
        valid: true,
        data: filters
    };
};
```

---

#### 2. AJV (Fastest - Standards-Based)

**Overview**: High-performance JSON Schema validator with compile-time optimization for speed.

**Stats**:
- Performance: **~14M ops/sec** (fastest)
- Downloads: Very high in enterprise
- JSON Schema versions: Draft 4, 6, 7, 2019-09, 2020-12

**Pros**:
- ✅ **Blazingly fast** (~14M ops/sec)
- ✅ JSON Schema standards-compliant
- ✅ Schema compilation for optimization
- ✅ Custom keywords and formats
- ✅ Async validation support
- ✅ Small bundle size
- ✅ Standalone (framework-agnostic)
- ✅ Excellent for OpenAPI/microservices

**Cons**:
- ❌ No native TypeScript inference (requires separate tools)
- ❌ JSON Schema is verbose compared to Zod
- ❌ Steeper learning curve
- ❌ TypeScript support less intuitive

**Best For**: High-performance APIs, microservices, OpenAPI compliance, standards-first projects

**Code Example - Login Validation**:
```typescript
import Ajv from 'ajv';

const ajv = new Ajv();

// Define login schema (JSON Schema format)
const loginSchema = {
    type: 'object',
    required: ['user', 'password'],
    properties: {
        user: {
            type: 'string',
            minLength: 3,
            maxLength: 50,
            pattern: '^[a-zA-Z0-9_]+$'
        },
        password: {
            type: 'string',
            minLength: 4,
            maxLength: 100
        },
        rememberMe: {
            type: 'boolean'
        }
    },
    additionalProperties: false
};

// Compile schema for maximum performance
const validateLogin = ajv.compile(loginSchema);

// Usage
export async function login(data: unknown) {
    // Step 1: Schema validation (extremely fast)
    const valid = validateLogin(data);

    if (!valid) {
        return {
            success: false,
            errors: validateLogin.errors
        };
    }

    // Step 2: Custom async validation
    const user = await db.usuario.findUnique({
        where: { user: data.user }
    });

    if (!user) {
        return {
            success: false,
            message: "User does not exist"
        };
    }

    if (user.tipo === 'OPERACIONAL') {
        return {
            success: false,
            message: "This user type cannot log in"
        };
    }

    return { success: true };
}
```

**Code Example - Quotation Filter Validation**:
```typescript
const quotationFilterSchema = {
    type: 'object',
    properties: {
        idVendedor: {
            type: 'integer',
            exclusiveMinimum: 0
        },
        mesAno: {
            type: 'string',
            pattern: '^\\d{4}-(0[1-9]|1[0-2])$'
        },
        fornecedor: {
            type: 'string',
            maxLength: 100
        },
        statuses: {
            type: 'array',
            items: {
                type: 'string',
                enum: ['ativo', 'cancelado', 'expirado', 'fechado', 'perdido', 'replicado']
            }
        },
        semaforo: {
            type: 'integer',
            enum: [0, 1, 2, 3]
        },
        search: {
            type: 'string',
            maxLength: 100
        }
    },
    additionalProperties: false
};

const validateQuotationFilter = ajv.compile(quotationFilterSchema);

// For TypeScript type safety with AJV, you'd use a companion tool:
interface QuotationFilter {
    idVendedor?: number;
    mesAno?: string;
    fornecedor?: string;
    statuses?: Array<'ativo' | 'cancelado' | 'expirado' | 'fechado' | 'perdido' | 'replicado'>;
    semaforo?: 0 | 1 | 2 | 3;
    search?: string;
}

export async function filterQuotations(data: unknown): Promise<{
    valid: boolean;
    data?: QuotationFilter;
    errors?: any[];
}> {
    const valid = validateQuotationFilter(data);

    if (!valid) {
        return { valid: false, errors: validateQuotationFilter.errors };
    }

    return { valid: true, data: data as QuotationFilter };
}
```

---

#### 3. Valibot (Zod Alternative - Lightweight)

**Overview**: Modern TypeScript validation library with modular design, 98% smaller bundle than Zod.

**Stats**:
- Bundle Size: **98% smaller** than Zod
- Performance: Comparable to Zod (~2M ops/sec)
- Adoption: Growing rapidly (v1 RC released)
- Tree-shakeable

**Pros**:
- ✅ **Tiny bundle size** (5-8KB vs Zod's 50KB)
- ✅ TypeScript-first with excellent inference
- ✅ Modular design (import only what you need)
- ✅ Pipes for transformations
- ✅ Standard Schema spec compliant
- ✅ Async validation support
- ✅ JSON Schema conversion
- ✅ Works with React Hook Form, tRPC

**Cons**:
- ❌ Smaller ecosystem than Zod
- ❌ Less mature
- ❌ Fewer third-party integrations
- ❌ Community still growing

**Best For**: Frontend applications, serverless functions, bundle-size-sensitive projects

**Code Example - Login Validation**:
```typescript
import * as v from 'valibot';

// Define login schema
const LoginSchema = v.object({
    user: v.pipe(
        v.string(),
        v.minLength(3, "Must be at least 3 characters"),
        v.maxLength(50, "Cannot exceed 50 characters"),
        v.regex(/^[a-zA-Z0-9_]+$/, "Only alphanumeric and underscore allowed")
    ),
    password: v.pipe(
        v.string(),
        v.minLength(4, "Must be at least 4 characters"),
        v.maxLength(100, "Cannot exceed 100 characters")
    ),
    rememberMe: v.optional(v.boolean(), false)
});

type LoginInput = v.InferInput<typeof LoginSchema>;
type LoginOutput = v.InferOutput<typeof LoginSchema>;

// Usage
export async function validateLogin(data: unknown) {
    try {
        const validated = await v.parseAsync(LoginSchema, data);

        // Custom async validation
        const user = await db.usuario.findUnique({
            where: { user: validated.user }
        });

        if (!user) {
            throw new Error("User does not exist");
        }

        if (user.tipo === 'OPERACIONAL') {
            throw new Error("This user type cannot log in");
        }

        return { success: true, data: validated };
    } catch (error) {
        if (error instanceof v.ValiError) {
            return {
                success: false,
                errors: error.issues.map(issue => ({
                    path: issue.path?.[0],
                    message: issue.message
                }))
            };
        }
        return {
            success: false,
            message: error instanceof Error ? error.message : 'Validation failed'
        };
    }
}
```

**Code Example - Quotation Filter Validation**:
```typescript
import * as v from 'valibot';

const QuotationFilterSchema = v.object({
    idVendedor: v.optional(
        v.pipe(
            v.number(),
            v.integer(),
            v.minValue(1)
        )
    ),
    mesAno: v.optional(
        v.pipe(
            v.string(),
            v.regex(/^\d{4}-(0[1-9]|1[0-2])$/, "Format must be YYYY-MM")
        )
    ),
    fornecedor: v.optional(
        v.pipe(v.string(), v.maxLength(100))
    ),
    statuses: v.optional(
        v.array(
            v.enum(['ativo', 'cancelado', 'expirado', 'fechado', 'perdido', 'replicado'])
        )
    ),
    semaforo: v.optional(
        v.enum([0, 1, 2, 3] as const)
    ),
    search: v.optional(
        v.pipe(v.string(), v.maxLength(100))
    )
});

type QuotationFilter = v.InferOutput<typeof QuotationFilterSchema>;

export async function filterQuotations(
    data: unknown
): Promise<{ valid: boolean; data?: QuotationFilter; errors?: any[] }> {
    try {
        const validated = await v.parseAsync(QuotationFilterSchema, data);
        return { valid: true, data: validated };
    } catch (error) {
        if (error instanceof v.ValiError) {
            return {
                valid: false,
                errors: error.issues
            };
        }
        return { valid: false };
    }
}
```

---

### Summary - TypeScript/JavaScript

| Feature | Zod | AJV | Valibot |
|---------|-----|-----|---------|
| **Downloads** | 64.5M/week | High | Growing |
| **Performance** | ~2M ops/sec | **~14M** | ~2M |
| **Bundle Size** | ~50KB | ~8KB | **~5-8KB** |
| **TS Inference** | Excellent | Via tools | Excellent |
| **Async Support** | ✅ | ✅ | ✅ |
| **Learning Curve** | Low-Medium | Medium-High | Low |
| **Ecosystem** | Large | Huge | Growing |
| **Best For** | Modern TS | Performance | Frontend |

**Recommendation for Staccato ERP**:
- **Full-stack TypeScript**: Zod (best ecosystem integration)
- **High-throughput APIs**: AJV (best performance)
- **Frontend-heavy**: Valibot (smallest bundle)
- **Shared schemas (tRPC)**: Zod

---

## Cross-Language Comparison

### Validation Features Matrix

| Feature | C++ (Qt) | .NET | PHP | TypeScript |
|---------|----------|------|-----|-----------|
| **Async Validation** | ❌ | ✅ (FluentValidation) | ✅ (closures) | ✅ (all major) |
| **Custom Rules** | ✅ (subclass) | ✅ (multiple ways) | ✅ (multiple ways) | ✅ (pipes/refine) |
| **Error Formatting** | Basic | Rich | Framework-dependent | Excellent (Zod) |
| **Performance** | Excellent | Good/Excellent | Good | Zod: 2M, AJV: 14M ops/sec |
| **Type Safety** | C++ types | Strong | Weak (PHP 7.4+) | TypeScript: Excellent |
| **Bundle Size** | N/A | N/A | N/A | Varies (5-50KB) |
| **Learning Curve** | Low-Medium | Low-Medium | Low | Low-Medium |
| **Brazilian Validation** | Custom | Custom | Respect/Validation | Custom |

### Architecture Recommendations

#### Frontend (TypeScript React)
```
Login Form → Zod Schema → API Request
    ↓
Server-side validation (see below)
```

#### Backend Validation Layers
```
Request → Framework Validator → Custom Rules → Database Constraints
```

**By Language**:
- **C++**: QValidator (UI) → cpp-validator (business logic) → Database CHECK constraints
- **.NET**: DataAnnotations (simple) or FluentValidation (complex) → Database constraints
- **PHP**: Framework validator (Laravel/Symfony) → Database constraints
- **TypeScript**: Zod/Valibot (form) → AJV (API) → Database constraints

---

## Recommendations for Staccato ERP

### Implementation Strategy

#### 1. **Login Page Validation**

**C++ (Qt)**:
```cpp
// UI Layer: QValidator for field constraints
userInput->setValidator(new QRegularExpressionValidator(QRegularExpression("^[a-zA-Z0-9_]{3,50}$"), this));
passwordInput->setValidator(new PasswordValidator(this));

// Business Layer: Custom user lookup and validation
```

**.NET**:
```csharp
// Use FluentValidation for async user existence check
RuleFor(x => x.Username)
    .MustAsync(UserMustExist, "User not found")
```

**Laravel**:
```php
// Form Request with custom validation rule
RuleFor('user')->custom(function($value) {
    if (!Usuario::where('user', $value)->exists()) {
        throw new \Illuminate\Validation\ValidationException('User not found');
    }
})
```

**TypeScript**:
```typescript
// Zod with async refinement
const LoginSchema = z.object({
    user: z.string().min(3).refine(
        async (u) => !!(await db.usuario.findUnique({ where: { user: u } })),
        "User does not exist"
    )
})
```

#### 2. **Quotation Filter Validation**

**C++ (Qt)**:
- Table model validation for selected rows
- Regex validators for date/numeric fields
- Proxy model for filtering

**.NET**:
- FluentValidation for complex filter combinations
- Guard clauses for null checks

**Laravel**:
- Form Request with built-in unique/exists rules
- Custom validation for status enums

**TypeScript**:
- Zod for form data
- AJV for API response validation

### Performance Prioritization

```
High-Throughput Quotation Queries:
.NET: Use Validot (13.3x better memory)
TypeScript: Use AJV (14M ops/sec)
PHP: Use framework validators (close to native speed)
C++: All options fast enough
```

### Developer Experience Prioritization

```
Ease of Implementation:
.NET: FluentValidation (most ergonomic)
PHP: Framework validators (built-in)
TypeScript: Zod (perfect type inference)
C++: QValidator subclasses (Qt-integrated)
```

### Enterprise/Production Prioritization

```
Best for Long-term Maintenance:
.NET: FluentValidation (783.7M downloads, battle-tested)
PHP: Framework validators (Laravel/Symfony built-in support)
TypeScript: Zod (64.5M downloads, growing ecosystem)
C++: Mix of built-in QValidator + cpp-validator
```

---

## Implementation Checklist

- [ ] **Login Page**: Implement validation across all implementations
  - [ ] Client-side: HTML5 + framework validators
  - [ ] Server-side: Async user existence check
  - [ ] Database: NOT NULL constraints

- [ ] **Quotation Filters**: Implement comprehensive validation
  - [ ] Vendor ID: Exists in database
  - [ ] Month/Year: Valid YYYY-MM format
  - [ ] Statuses: Enum from predefined set
  - [ ] Semáforo: 0-3 range
  - [ ] Search: Max length enforcement

- [ ] **Shared Validation Rules**: Document across implementations
  - [ ] Brazilian CPF/CNPJ validation (if needed)
  - [ ] Business day/month calculations
  - [ ] Status transition rules

- [ ] **Error Handling**: Consistent across implementations
  - [ ] Structured error responses
  - [ ] Field-level error messages
  - [ ] Localization support

---

## References & Resources

### C++ (Qt 5.15)
- Qt Validator Documentation: https://doc.qt.io/qt-6/qvalidator.html
- cpp-validator GitHub: https://github.com/evgeniums/cpp-validator
- Valijson GitHub: https://github.com/tristanpenman/valijson

### .NET 8
- FluentValidation: https://docs.fluentvalidation.net/
- Validot GitHub: https://github.com/bartoszlenar/Validot
- DataAnnotations: https://learn.microsoft.com/en-us/dotnet/api/system.componentmodel.dataannotations

### PHP
- Laravel Validation: https://laravel.com/docs/validation
- Symfony Validator: https://symfony.com/doc/current/validation.html
- Respect/Validation: https://respect-validation.readthedocs.io/

### TypeScript
- Zod: https://zod.dev/
- AJV: https://ajv.js.org/
- Valibot: https://valibot.dev/
- Yup: https://github.com/jquense/yup
- Joi: https://joi.dev/

---

**Document Version**: 1.0
**Last Updated**: December 2025
**Scope**: Staccato ERP Project - Validation Library Selection Guide
