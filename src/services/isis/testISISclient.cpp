/* Test ISIS client for the following operations:
    Query - for querying remote ISIS
        attributes: The query string.
    Register - for sending test Register messages
        attributes:
            The ServiceID for register.
            A key - value pair for register.
    RemoveRegistration - for sending test RemoveRegistration messages
        attributes: The ServiceID for remove.
    (The GetISISList operation will be used in every other cases impicitly.)

    Usage examples:
        testISISclient Query "query string"
        testISISclient RemoveRegistration "ServiceID"
        etc.
*/
