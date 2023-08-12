<?php

function services_security_admin_form($form, &$form_state) {
  // Check to see if anything has been stored.
  if ($form_state['rebuild']) {
    $form_state['input'] = array();
  }
  if (empty($form_state['storage'])) {
    // No step has been set so start with the first.
    $form_state['storage'] = array(
      'step' => 'services_security_form_decision',
    );
  }

   // Return the current form
  $function = $form_state['storage']['step'];
  $form = $function($form, $form_state);
  return $form;
}
function services_security_form_decision($form, &$form_state) {
  $values = '';
  if(!empty($form_state['storage'])) {
      $values = $form_state['storage'];
  }
  $notice = '<div style="color:red;"><strong>A Services security update mitigation step has already been run on this site.</strong></div>';
  $services_security_update = config_get('services.settings', 'services_security_update_1');
  //If services security has not run before, lets set the notice to nothing.
  if(!$services_security_update) {
    $notice = '';
  }
  $form['markup'] = array(
    '#markup' => $notice . 'Due to a bug in services, user accounts registered through services\' user_resource have been created with the password "1" since August 2013.

    <p>Services provides the following options to mitigate this vulnerability on your site:
      <ol>
        <li>Invalidate the password of all user accounts that have been registered after this bug was introduced. This will force all users who registered after August 30th, 2013 to reset their password, regardless of how those accounts were created. <strong>This is the safest option</strong>.</li>
        <li>Invalidate the password of all user accounts which currently have their password set to "1". This will require users who attempted to register to reset their password.
        This option will take a long time to run especially if you have a lot of users on your site.
        <strong style="color:red;">This option may not be effective from a security perspective because an attacker may have already changed passwords to something other than "1".</strong></li>
        <li>Do nothing.</li>
      </ol>
    </p>

    <p>There are many reasons why the third option (do nothing) would be suitable to you:
      <ol>
        <li>Services User Resource was never enabled</li>
        <li>Anonymous users did not have permission to register</li>
        <li>A custom/contrib resource was enabled that users used in order to register</li>
        <li>You have an SSO provider and users do not register through Services</li>
        <li>Users were never registered through Services because the API was not public</li>
        <li>You were using a version of Services older than 7.x-3.6 and never used Services 7.x-3.6 on your site.</li>
      </ol>
    </p>

      <p><strong>Things you should do as general best practices:</strong>
      <ol>
        <li>Check all accounts that have administrator access and verify they are accounts you know. If not, its recommended to disable those accounts</li>
        <li>If you choose option 1 or 2 you should let your users know that they will need to request a password reset via the regular form at user/password.</li>
      </ol></p>',
  );
  $form['fieldset'] = array(
   '#type' => 'fieldset',
   '#title' => t('I understand. Let\'s do something about it!'),
   '#collapsible' => TRUE,
   '#collapsed' => TRUE,
  );
  $form['fieldset']['security_options'] = array(
    '#type'     => 'radios',
    '#title'    => t('Please select from the following options'),
    '#options'  => array(
      t('Invalidate password of all user accounts created after August 30th, 2013 (safest)'),
      t('Invalidate password of all user accounts with a password of "1".'),
      t('Do nothing'),
    ),
    '#default_value' => isset($values['security_options']) ? $values['security_options'] : 2,
    '#required' => TRUE,
  );

  $form['fieldset']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );
  if(isset($form_state['decided_security_option'])) {
    unset($form['fieldset']);
  }
  return $form;
}
function services_security_admin_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  if (isset($values['back']) && $values['op'] == $values['back']) {
    // Moving back in form.
    $step = $form_state['storage']['step'];
    // Call current step submit handler if it exists to unset step form data.
    if (function_exists($step . '_submit')) {
      $function = $step . '_submit';
      $function($form, $form_state);
    }
    // Remove the last saved step so we use it next.
    $last_step = array_pop($form_state['storage']['steps']);
    $form_state['storage']['step'] = $last_step;
  }
  else {
    // Record step.
    $step = $form_state['storage']['step'];
    $form_state['storage']['steps'][] = $step;
    // Call step submit handler if it exists.
    if (function_exists($step . '_submit')) {
      $function = $step . '_submit';
      $function($form, $form_state);
    }
  }
  return;
}

function services_security_form_confirm($form, &$form_state) {
  $values = array();
  if (!empty($form_state['storage'])) {
    $values = $form_state['storage'];
  }
  switch($values['security_options']) {
    case 0:
      $markup = 'All user account created since August 30th, 2013 will have their password invalidated, this cannot be undone.';
      break;
    case 1:
      $markup = 'All user account which still have their password set to "1" will have their password invalidated, this cannot be undone.';
      break;
    case 2:
      $markup = 'Do nothing.';
      break;
  }

  $form['markup'] = array(
    '#markup' => $markup . '<br>',
  );
  $form['back'] = array(
    '#type' => 'submit',
    '#value' => t('Back'),
    '#limit_validation_errors' => array(),
    '#submit' => array('services_security_form_confirm_submit'),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Confirm'),
  );
  return $form;
}

function services_security_form_confirm_submit($form, &$form_state) {
  $values = $form_state['values'];
  $form_state['rebuild'] = TRUE;

  //If they hit back, lets send them back.
  if (isset($values['back']) && $values['op'] == $values['back']) {
    $form_state['storage']['step'] = 'services_security_form_decision';
  } else {
    //Setup the batch processing.
    $security_options = services_security_get_update_options();
    $op = $security_options[$form_state['storage']['security_options']];
    services_security_setup_batch($op, FALSE);
  }
}
//
function services_security_form_decision_submit($form, &$form_state) {
  $values = $form_state['values'];
  $form_state['rebuild'] = TRUE;
  $form_state['storage']['security_options'] = check_plain($form_state['values']['security_options']);
  $form_state['storage']['step'] = 'services_security_form_confirm';
}

function theme_services_resource_table($variables) {
  $table = $variables['table'];

  backdrop_add_css(backdrop_get_path('module', 'services') . '/css/services.admin.css');
  backdrop_add_js(backdrop_get_path('module', 'services') . '/js/services.admin.js');
  backdrop_add_js('core/misc/tableselect.js');

  // Create header for resource selection table.
  $header = array(
    array('class' => array('select-all')),
    array('data' => t('Resource'), 'class' => array('resource_method')),
    array('data' => t('Settings'), 'class' => array('resource_settings')),
    array('data' => t('Alias'), 'class' => array('resource_alias')),
  );

  // Define the images used to expand/collapse the method groups.
  $js = array(
    'images' => array(
      'collapsed' => theme('image', array('uri' => 'core/misc/menu-collapsed.png', 'alt' => t('Expand'), 'title' => t('Expand'))) . ' <a href="#" class="resource-collapse">(' . t('Expand') . ')</a>',
      'expanded' => theme('image', array('uri' => 'core/misc/menu-expanded.png', 'alt' => t('Collapse'), 'title' => t('Collapse'))) . ' <a href="#" class="resource-collapse">(' . t('Collapse') . ')</a>',
    ),
  );

  // Cycle through each method group and create a row.
  $rows = array();
  foreach (element_children($table) as $key) {
    $element = &$table[$key];
    $row = array();

    // Make the class name safe for output on the page by replacing all
    // non-word/decimal characters with a dash (-).
    $method_class = 'services-' . strtolower(trim(preg_replace("/[^\w\d]/", "-", $key)));

    // Select the right "expand"/"collapse" image, depending on whether the
    // category is expanded (at least one method selected) or not.
    $collapsed = !empty($element['#collapsed']);

    // Place-holder for checkboxes to select group of methods.
    $row[] = array('id' => $method_class, 'class' => array('resource-select-all'));

    // Expand/collapse image and group title.
    $row[] = array(
      'data' => '<div class="resource-image" id="resource-method-group-' . $method_class . '" data-resource="' . $method_class . '"></div>' .
        '<label for="' . $method_class . '-select-all" class="resource-group-label">' . $key . '</label>',
      'class' => array('resource-group-label'),
    );

    $row[] = array(
      'data' => '&nbsp;',
      'class' => array('resource-group-description'),
    );
    $row[] = array(
      'data' => backdrop_render($element['alias']),
      'class' => array('resource-group-alias'),
    );
    $rows[] = array('data' => $row, 'class' => array('resource-group'));

    // Add individual methods to group.
    $current_js = array(
      'methodClass' => $method_class . '-method',
      'collapsed' => $collapsed,
      'clickActive' => FALSE,
    );

    // Cycle through each method within the current group.
    foreach (element_children($element) as $class) {
      if($class != 'alias') {
        $class_element = $element[$class];

        // Add group (class) header row.
        $rows[] = array('data' => array(NULL, array(
          'data' => '<label>' . $class_element['#title'] . '</label>',
          'class' => array('resource-operation-class'),
        ), NULL, NULL), 'class' => array($method_class . '-method', 'resource-operation-class'));

        foreach (element_children($class_element) as $op_name) {
          $row = array();
          $method = $class_element[$op_name];

          // Store method title and description so that checkbox won't render them.
          $title = $method['#title'];
          $description = $method['#description'];

          $method['#title_display'] = 'invisible';
          $method['enabled']['#title_display'] = 'invisible';
          unset($method['#description']);

          // Test name is used to determine what methods to run.
          $method['#name'] = $class;

          $row[] = array(
            'data' => backdrop_render($method['enabled']),
            'class' => array('resource-method-select'),
          );
          $row[] = array(
            'data' => '<label for="' . $method['#id'] . '">' . $title . '</label>' . '<div class="description">' . $description . '</div>',
            'class' => array('resource-method-description'),
          );
          $row[] = array(
            'data' => backdrop_render($method['settings']),
            'class' => array('resource-method-settings'),
          );
          $row[] = array(
            'data' => '<div class="alias">&nbsp;</div>',
            'class' => array('resource-method-alias'),
          );
          $rows[] = array('data' => $row, 'class' => array($method_class . '-method', 'resource-method'));
        }
      }

    }
    $js['resources'][$method_class] = $current_js;
    unset($table[$key]);
  }

  // Add js array of settings.
  backdrop_add_js(array('services' => $js), 'setting');

  if (empty($rows)) {
    return '<strong>' . t('No resourcess to display.') . '</strong>';
  }
  else {
    return theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => 'resource-form-table')));
  }
}

function services_list_page() {
  $endpoints = services_endpoint_load_all();
  // backdrop_sort($endpoints, array('title' => SORT_STRING));
  $header = array(t('Name'), t('Path'), t('Operations'));
  $ops = array(
    'edit'   => t('Edit'),
    'clone' => t('Clone'),
    'delete'   => t('Delete'),
    'resources'  => t('Edit Resources'),
    'server'  => t('Edit Server'),
    'authentication' => t('Edit Authentication'),
  );

  $rows = array();
  foreach ($endpoints as $endpoint) {
    if ($endpoint->status) {
      $ops['disable'] = t('Disable');
    }
    else {
      $ops['enable'] = t('Enable');
    }
    $row = array();
    $row[] = $endpoint->name;
    $row[] = $endpoint->path;
    $links = array();
    foreach ($ops as $key => $op) {
      $links[$key] = array(
        'title' => $op,
        'href' => 'admin/structure/services/list/' . $endpoint->name . '/' . $key,
      );
    }
    $row[] = array(
      'data' => array(
        '#type' => 'operations',
        '#links' => $links,
      ),
    );
    $rows[] = $row;
  }

  return theme('table', array('header' => $header, 'rows' => $rows));
}

/**
 * Form to edit the settings of an endpoint.
 */
function services_endpoint_edit_form($form, &$form_state, $endpoint = NULL) {
  // Loading runtime include as needed by services_auth_info().
  module_load_include('inc', 'services', 'includes/services.runtime');

  $form = array();
  $form['name'] = array(
    '#type' => 'machine_name',
    '#title' => t('Machine-readable name of the endpoint'),
    '#machine_name' => array(
      'exists' => 'services_endpoint_machine_name_exists',
    ),
    '#maxlength' => 128,
    '#default_value' => isset($endpoint->name) ? $endpoint->name : '',
    '#required' => TRUE,
    '#description' => t('The endpoint name can only consist of lowercase letters, underscores, and numbers.'),
  );

  $form['endpoint_object'] = array(
    '#type'  => 'value',
    '#value' => $endpoint,
  );

  $servers = services_get_servers();
  $server_opts = array(
    '' => t('-- Select a server'),
  );
  foreach ($servers as $server => $info) {
    $server_opts[$server] = $info['name'];
  }
  $form['server'] = array(
    '#type'          => 'select',
    '#options'       => $server_opts,
    '#default_value' => isset($endpoint->server) ? $endpoint->server : '',
    '#title'         => t('Server'),
    '#description'   => t('Select the server that should be used to handle requests to this endpoint.'),
    '#required'      => TRUE,
  );

  $form['path'] = array(
    '#type'          => 'textfield',
    '#size'          => 24,
    '#maxlength'     => 255,
    '#default_value' => isset($endpoint->path) ? $endpoint->path : '',
    '#title'         => t('Path to endpoint'),
    '#required'      => TRUE,
  );

  $form['debug'] = array(
    '#type'          => 'checkbox',
    '#default_value' => isset($endpoint->debug) ? $endpoint->debug : '',
    '#title'         => t('Debug mode enabled'),
    '#description'   => t('Useful for developers. Do not enable on production environments'),
    '#required'      => FALSE,
  );
  $auth_modules = module_implements('services_authentication_info');
  if (!empty($auth_modules)) {
    $auth_options = array();
    foreach ($auth_modules as $module) {
      $info = services_authentication_info($module);
      $auth_options[$module] = $info['title'];
    }
    $default_values = array();
    if ($endpoint) {
      foreach ($endpoint->authentication as $auth_module => $settings) {
        if (!empty($settings)) {
          $default_values[] = $auth_module;
        }
      }
    }

    $form['authentication'] = array(
      '#type'          => 'checkboxes',
      '#options'       => $auth_options,
      '#default_value' => $default_values,
      '#title'         => t('Authentication'),
      '#description'   => t('Choose which authentication schemes that should ' .
        'be used with your endpoint. If no authentication method is selected ' .
        'all requests will be done by an anonymous user.'),
    );
  }
  else {
    $form['authentication'] = array(
      '#type'          => 'item',
      '#title'         => t('Authentication'),
      '#description'   => t('No authentication modules are installed, all ' .
        'requests will be anonymous.'),
    );
  }

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => empty($endpoint) ? t('Save endpoint') : t('Update endpoint'),
    '#validate' => array(
      'services_endpoint_edit_form_validate',
    ),
    '#submit' => array(
      'services_endpoint_edit_form_submit',
    ),
  );

  return $form;
}

/**
 * Validate submission of the preset edit form.
 */
function services_endpoint_edit_form_validate(&$form, &$form_state) {
  $endpoint_object = $form_state['values']['endpoint_object'];

  // Evaluate the endpoint path if this endpoint is new or the path has changed.
  if (!$endpoint_object || $endpoint_object->is_new || $endpoint_object->path != $form_state['values']['path']) {
    $names = config_get_names_with_prefix('services.endpoint.');
    foreach ($names as $config_name) {
      $config = config($config_name);
      $config_path = $config->get('path');

      if ($form_state['values']['path'] == $config_path) {
        form_error($form['path'], t('Endpoint path must be unique.'));
      }
    }
  }
}

/**
 * Endpoint name check whether this machine name already exists.
 */
function services_endpoint_machine_name_exists($value) {
  $result = config_get('services.endpoint.' . $value, 'name');
  return ($result == $value);
}

/**
 * Submit handler for endpoint.
 */
function services_endpoint_edit_form_submit(&$form, &$form_state) {
  $endpoint = $form_state['values']['endpoint_object'];
  if (empty($endpoint)) {
    $endpoint = new stdClass;
  }
  if (!$endpoint->is_new && $endpoint->name && $endpoint->name != $form_state['values']['name']) {
    // Endpoint is being renamed. Remove the old config file.
    $config_old = config('services.endpoint.' . $endpoint->name);
    $config_old->delete();
  }

  $endpoint->name   = $form_state['values']['name'];
  $endpoint->server = $form_state['values']['server'];
  $endpoint->path   = $form_state['values']['path'];
  $endpoint->debug  = $form_state['values']['debug'];

  // Set the authentication modules, and preserve the settings for modules
  // that already exist.
  $auth = array();
  if (isset($form_state['values']['authentication'])) {
    foreach (array_keys($form_state['values']['authentication']) as $module) {
      //if module's checkbox is checked, add to empty
      $auth_module = $form_state['values']['authentication'][$module];
      if ($module === $auth_module) {
        //If existing settings are set, preserve them
        if (isset($endpoint->authentication[$module]) && is_array($endpoint->authentication[$module]) && !empty($endpoint->authentication[$module])) {
          $auth[$module] = $endpoint->authentication[$module];
        }
        else {
          $auth[$module] = $auth_module;
        }
      }
      elseif ($auth_module == 0) {
        unset($auth[$module]);
      }
    }
  }
  $endpoint->authentication = $auth;
  $endpoint->is_new = TRUE;

  services_endpoint_save($endpoint);
  $form_state['redirect'] = 'admin/structure/services';
}

/**
 * Endpoint authentication configuration form.
 */
function services_edit_form_endpoint_authentication($form, &$form_state, $endpoint) {
  // Loading runtime include as needed by services_authentication_info().
  module_load_include('inc', 'services', 'includes/services.runtime');

  $auth_modules = module_implements('services_authentication_info');

  $form['endpoint_object'] = array(
    '#type'  => 'value',
    '#value' => $endpoint,
  );
  if (empty($auth_modules)) {
    $form['message'] = array(
      '#type'          => 'item',
      '#title'         => t('Authentication'),
      '#description'   => t('No authentication modules are installed, all requests will be anonymous.'),
    );
    return $form;
  }
  if (empty($endpoint->authentication)) {
    $form['message'] = array(
      '#type'          => 'item',
      '#title'         => t('Authentication'),
      '#description'   => t('No authentication modules are enabled, all requests will be anonymous.'),
    );
    return $form;
  }
  // Add configuration fieldsets for the authentication modules
  foreach ($endpoint->authentication as $module => $settings) {
    $info = services_authentication_info($module);
    if (empty($info)) {
      continue;
    }
    $form[$module] = array(
      '#type' => 'fieldset',
      '#title' => isset($info['title']) ? $info['title'] : $module,
      '#tree' => TRUE,
    );

    // Append the default settings for the authentication module.
    $default_security_settings = services_auth_invoke($module, 'default_security_settings');
    if ($settings == $module && is_array($default_security_settings)) {
      $settings = $default_security_settings;
    }
    // Ask the authentication module for a settings form.
    $module_settings_form = services_auth_invoke($module, 'security_settings', $settings, $form_state);

    if (is_array($module_settings_form)) {
      $form[$module] += $module_settings_form;
    }
    else {
      $form[$module]['message'] = array(
        '#type'   => 'item',
        '#markup'  => t('@module has no settings available.', array('@module' => backdrop_ucfirst($module))),
      );
    }
  }

  $form['submit'] = array(
    '#type'  => 'submit',
    '#value' => 'Save',
  );

  return $form;
}

function services_edit_form_endpoint_authentication_submit($form, $form_state) {
  $endpoint = $form_state['values']['endpoint_object'];

  foreach (array_keys($endpoint->authentication) as $module) {
    if (isset($form_state['values'][$module])) {
      $endpoint->authentication[$module] = $form_state['values'][$module];
    }
  }

  backdrop_set_message(t('Your authentication options have been saved.'));
  services_endpoint_save($endpoint);
}

function services_edit_form_endpoint_server($form, &$form_state, $endpoint) {
  $servers = services_get_servers();
  $server = !empty($servers[$endpoint->server]) ? $servers[$endpoint->server] : FALSE;

  $form['endpoint_object'] = array(
    '#type'  => 'value',
    '#value' => $endpoint,
  );

  if (!$server) {
    $form['message'] = array(
      '#type'          => 'item',
      '#title'         => t('Unknown server @name', array('@name' => $endpoint->server)),
      '#description'   => t('No server matching the one used in the endpoint.'),
    );
  }
  else if (empty($server['settings'])) {
    $form['message'] = array(
      '#type'          => 'item',
      '#title'         => t('@name has no settings', array('@name' => $endpoint->server)),
      '#description'   => t("The server doesn't have any settings that needs to be configured."),
    );
  }
  else {
    $definition = $server['settings'];

    $settings = isset($endpoint->server_settings) ? $endpoint->server_settings : array();

    if (!empty($definition['file'])) {
      call_user_func_array('module_load_include', $definition['file']);
    }

    $form[$endpoint->server] = array(
      '#type' => 'fieldset',
      '#title' => $server['name'],
      '#tree' => TRUE,
    );
    call_user_func_array($definition['form'], array(&$form[$endpoint->server], $endpoint, $settings));

    $form['submit'] = array(
      '#type'  => 'submit',
      '#value' => 'Save',
    );
  }

  return $form;
}

function services_edit_form_endpoint_server_submit($form, $form_state) {
  $endpoint = $form_state['values']['endpoint_object'];
  $servers = services_get_servers();
  $definition = $servers[$endpoint->server]['settings'];

  $values = $form_state['values'][$endpoint->server];

  // Allow the server to alter the submitted values before they're stored
  // as settings.
  if (!empty($definition['submit'])) {
    if (!empty($definition['file'])) {
      call_user_func_array('module_load_include', $definition['file']);
    }
    $values = call_user_func_array($definition['submit'], array($endpoint, &$values));
  }

  // Store the settings in the endpoint
  $endpoint->server_settings = $values;
  services_endpoint_save($endpoint);

  backdrop_set_message(t('Your server settings have been saved.'));
}

/**
 * services_edit_endpoint_resources function.
 *
 * Edit Resources endpoint form
 * @param object $endpoint
 * @return string  The form to be displayed
 */
function services_edit_endpoint_resources($endpoint) {
  if (!is_object($endpoint)) {
    $endpoint = services_endpoint_load($endpoint);
  }
  if ($endpoint && !empty($endpoint->title)) {
    backdrop_set_title($endpoint->title);
  }
  return backdrop_get_form('services_edit_form_endpoint_resources', $endpoint);
}

/**
 * services_edit_form_endpoint_resources function.
 *
 * @param array &$form_state
 * @param object $endpoint
 * @return Form
 */
function services_edit_form_endpoint_resources($form, &$form_state, $endpoint) {
  module_load_include('inc', 'services', 'includes/services.resource_build');
  module_load_include('inc', 'services', 'includes/services.runtime');

  $form = array();
  $form['endpoint_object'] = array(
    '#type'  => 'value',
    '#value' => $endpoint,
  );

  $form['#attached']['js'] = array(
    'core/misc/tableselect.js',
    backdrop_get_path('module', 'services') . '/js/services.admin.js',
  );

  $form['#attached']['css'] = array(
    backdrop_get_path('module', 'services') . '/css/services.admin.css',
  );

  $ops = array(
    'create'   => t('Create'),
    'retrieve' => t('Retrieve'),
    'update'   => t('Update'),
    'delete'   => t('Delete'),
    'index'    => t('Index'),
  );

  // Call _services_build_resources() directly instead of
  // services_get_resources to bypass caching.
  $resources = _services_build_resources($endpoint->name);
  // Sort the resources by the key, which is the string used for grouping each
  // resource in theme_services_resource_table().
  ksort($resources);

  $form['instructions'] = array(
    '#type' => 'item',
    '#title' => t('Resources'),
    '#description' => t('Select the resource(s) or methods you would like to enable, and click <em>Save</em>.'),
   );

  $form['resources']= array(
    '#theme' => 'services_resource_table',
    '#tree' => TRUE,
   );

  $class_names = services_operation_class_info();
  // Collect authentication module info for later use and
  // append the default settings for authentication modules
  $auth_info = array();
  foreach ($endpoint->authentication as $module => $settings) {
    $auth_info[$module] = services_authentication_info($module);

    // Append the default settings for the authentication module.
    $default_settings = services_auth_invoke($module, 'default_security_settings');
    if (is_array($default_settings) && is_array($settings)) {
      $settings += $default_settings;
    }
    $endpoint->authentication[$module] = $settings;
  }
  // Generate the list of methods arranged by resource.
  foreach ($resources as $resource_name => $resource) {
    $resource_conf = array();
    $resource_key = $resource['key'];
    if (isset($endpoint->resources[$resource_key])) {
      $resource_conf = $endpoint->resources[$resource_key];
    }

    $res_item = array(
      '#collapsed' => TRUE,
    );
    $alias = '';
    if (isset($form_state['input'][$resource_key]['alias'])) {
      $alias = $form_state['input'][$resource_key]['alias'];
    }
    elseif (isset($resource_conf['alias'])) {
      $alias = $resource_conf['alias'];
    }

    $res_item['alias'] = array(
      '#type' => 'textfield',
      '#default_value' => $alias,
      '#size' => 20,
    );
    foreach ($class_names as $class => $info) {
      if (!empty($resource[$class])) {
        $res_item[$class] = array(
          '#type' => 'item',
          '#title' => $info['title'],
        );
        foreach ($resource[$class] as $op_name => $op) {
          $description = isset($op['help']) ? $op['help'] : t('No description is available');
          $default_value = 0;
          if (isset($resource_conf[$class][$op_name]['enabled'])) {
            $default_value = $resource_conf[$class][$op_name]['enabled'];
          }
          // If any component of a resource is enabled, expand the resource.
          if ($default_value) {
            $res_item['#collapsed'] = FALSE;
          }
          $res_item[$class][$op_name] = array(
            '#type' => 'item',
            '#title' => $op_name,
            '#description' => $description,
          );
          $res_item[$class][$op_name]['enabled'] = array(
            '#type' => 'checkbox',
            '#title' => t('Enabled'),
            '#default_value' => $default_value,
          );

          $controller_settings = array();
          $info = array(
            'op' => $op + array('name' => $op_name),
            'class' => $class,
            'resource' => $resource,
            'endpoint' => $endpoint,
          );

          // Let modules add their own settings.
          backdrop_alter('controller_settings', $controller_settings, $info);

          // Get service update versions.
          $update_versions = services_get_update_versions($resource_key, $op_name);
          $options = array(
            '1.0' => '1.0',
          );
          $options = array_merge($options, $update_versions);
          $default_api_value = 0;

          if (isset($op['endpoint']) && isset($op['endpoint']['services'])) {
            $default_api_value = $op['endpoint']['services']['resource_api_version'];
          }
          // Add the version information if it has any
          if (count($options) !== 1) {
            $controller_settings['services'] = array(
              '#title' => 'Services',
              '#type' => 'item',
              'resource_api_version' => array(
                '#type' => 'select',
                '#options' => $options,
                '#default_value' => isset($resource_conf[$class][$op_name]['settings']['services']['resource_api_version']) ? $resource_conf[$class][$op_name]['settings']['services']['resource_api_version'] : $options['1.0'],
                '#title' => 'Resource API Version',
              ),
            );
          }
          foreach ($endpoint->authentication as $module => $settings) {
            if (isset($endpoint->resources[$resource_key][$class][$op_name]['settings'][$module])) {
              $settings = $endpoint->resources[$resource_key][$class][$op_name]['settings'][$module];
            }
            $auth_settings = services_auth_invoke($module, 'controller_settings', $settings, $op, $endpoint->authentication[$module], $class, $op_name);
            if (is_array($auth_settings)) {
              $auth_settings = array(
                '#title' => $auth_info[$module]['title'],
                '#type' => 'item',
              ) + $auth_settings;
              $controller_settings[$module] = $auth_settings;
              $disabled = FALSE;
            }
          }
          if (!empty($controller_settings)) {
            $res_item[$class][$op_name]['settings'] = $controller_settings;
          }
        }
      }
    }
    $form['resources'][$resource_key] = $res_item;
  }
  $form['save'] = array(
     '#type'  => 'submit',
     '#value' => t('Save'),
  );
  return $form;
}

/**
 * services_edit_form_endpoint_resources_validate function.
 *
 * @param array $form
 * @param array $form_state
 * @return void
 */
function services_edit_form_endpoint_resources_validate($form, $form_state) {
  $input = $form_state['values'];

  // Validate aliases.
  foreach ($input['resources'] as $resource_name => $resource) {
    if (!empty($resource['alias']) && !preg_match('/^[0-9a-z-_]+$/', $resource['alias'])) {
      // Still this doesn't highlight needed form element.
      form_set_error("resources][{$resource_name}][alias", t("The alias for the !name resource may only contain lower case a-z, digits 0-9, underscores and dashes.", array(
        '!name' => $resource_name,
      )));
    }
  }
}

/**
 * Resources form submit function.
 *
 * @param array $form
 * @param array $form_state
 * @return void
 */
function services_edit_form_endpoint_resources_submit($form, $form_state) {
  $endpoint  = $form_state['values']['endpoint_object'];
  $resources = $form_state['input']['resources'];
  $class_names = services_operation_class_info();
  // Iterate over the resources, its operation classes and operations.
  // The main purpose is to remove empty configuration for disabled elements.
  foreach ($resources as $resource_name => $resource) {
    if (empty($resource['alias'])) {
      unset($resource['alias']);
    }
    foreach ($class_names as $class_name => $info) {
      if (!empty($resource[$class_name])) {
        foreach ($resource[$class_name] as $op_name => $op) {
          // Remove the operation if it has been disabled.
          if (!$op['enabled']) {
            unset($resource[$class_name][$op_name]);
          }
        }
      }
      // Remove the operation class element if it doesn't
      // have any enabled operations.
      if (empty($resource[$class_name])) {
        unset($resource[$class_name]);
      }
    }
    // Remove the resource if it doesn't have any properties.
    if (empty($resource)) {
      unset($resources[$resource_name]);
    }
    // Add the processed resource if it does.
    else {
      $resources[$resource_name] = $resource;
    }
  }
  $endpoint->resources = $resources;
  services_endpoint_save($endpoint);
  backdrop_set_message('Resources have been saved');
}

/**
 * Returns the updates for a given resource method.
 *
 * @param $resource
 *   A resource name.
 * @param $method
 *   A method name.
 * @return
 *   an array with the major and minor api versions
 */
function services_get_update_versions($resource, $method) {
  $versions = array();
  $updates = services_get_updates();
  if (isset($updates[$resource][$method]) && is_array($updates[$resource][$method])) {
    foreach ($updates[$resource][$method] as $update) {
      extract($update);
      $value = $major . '.' . $minor;
      $versions[$value] = $value;
    }
  }
  return $versions;
}

/**
 * Form callback; Copy an existing endpoint into a new endpoint.
 *
 * @ingroup forms
 */
function services_endpoint_clone_form($form, &$form_state, $endpoint) {
  $new_endpoint = clone($endpoint);
  $new_endpoint->is_new = TRUE;

  $form = services_endpoint_edit_form($form, $form_state, $new_endpoint);
  $form['name']['#default_value'] = 'clone_of_' . $form['name']['#default_value'];
  $form['actions']['submit']['#value'] = t('Clone endpoint');

  return $form;
}

/**
 * Form callback; Disable or enable an endpoint.
 *
 * @ingroup forms
 */
function services_toggle_enable_page($form, &$form_state, $endpoint) {
  $form_state['endpoint'] = $endpoint;
  if ($endpoint->status) {
    $question = t('Disable endpoint @title?', array('@title' => $endpoint->name));
    $text = t('Are you sure you want to disable this endpoint?');
    $button_text = t('Disable endpoint');
  }
  else {
    $question = t('Enable endpoint @title?', array('@title' => $endpoint->name));
    $text = t('Enable this endpoint?');
    $button_text = t('Enable endpoint');
  }

  return confirm_form($form, $question, 'admin/structure/services/list', $text, $button_text);
}

/**
 * Submit handler for services_toggle_enable_page_submit().
 */
function services_toggle_enable_page_submit($form, &$form_state) {
  $endpoint = $form_state['endpoint'];
  $status = $endpoint->status ? 0 : 1;
  config_set('services.endpoint.' . $endpoint->name, 'status', $status);
  backdrop_set_message($endpoint->name . ' was updated.');
  backdrop_goto('admin/structure/services/list');
}

/**
 * Form callback; Disable or enable an endpoint.
 *
 * @ingroup forms
 */
function services_endpoint_delete_form($form, &$form_state, $endpoint) {
  $form_state['endpoint'] = $endpoint;
  $question = t('Are you sure you wish to delete endpoint @title? This action cannot be undone.', array('@title' => $endpoint->name));
  $text = t('Delete this endpoint?');
  $button_text = t('Delete endpoint');

  return confirm_form($form, $question, 'admin/structure/services/list', $text, $button_text);
}

/**
 * Submit handler for services_toggle_enable_page_submit().
 */
function services_endpoint_delete_form_submit($form, &$form_state) {
  $endpoint = $form_state['endpoint'];
  services_endpoint_delete($endpoint);
  backdrop_set_message($endpoint->name . ' was deleted.');
  backdrop_goto('admin/structure/services/list');
}

