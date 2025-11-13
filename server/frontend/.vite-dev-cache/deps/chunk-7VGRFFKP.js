import {
  VBtn
} from "./chunk-25IQAKKZ.js";
import {
  touch_default
} from "./chunk-ZGADVLAM.js";
import {
  useSsrBoot
} from "./chunk-4R7HKKEK.js";
import {
  makeGroupItemProps,
  makeGroupProps,
  useGroup,
  useGroupItem
} from "./chunk-TGCHCPFR.js";
import {
  VDivider
} from "./chunk-SLLJ7KFV.js";
import {
  ripple_default
} from "./chunk-UMLPKMTR.js";
import {
  makeLazyProps,
  useLazy
} from "./chunk-TEKGGN5Z.js";
import {
  makeDisplayProps,
  useDisplay
} from "./chunk-GLV4GEKX.js";
import {
  VAvatar
} from "./chunk-UD6WDZ7V.js";
import {
  genOverlays
} from "./chunk-MYBNBMBC.js";
import {
  VSheet,
  makeVSheetProps
} from "./chunk-O6L4IA25.js";
import {
  VIcon
} from "./chunk-E7KYIGW2.js";
import {
  VDefaultsProvider
} from "./chunk-IM5AFMW4.js";
import {
  MaybeTransition
} from "./chunk-FW5XEMPP.js";
import {
  IconValue
} from "./chunk-2FTS52JW.js";
import {
  useLocale,
  useRtl
} from "./chunk-4XYP7SZ4.js";
import {
  makeTagProps
} from "./chunk-UVMYLE7E.js";
import {
  makeThemeProps,
  provideTheme
} from "./chunk-YBZL2HCC.js";
import {
  useProxiedModel
} from "./chunk-7GPVUA6G.js";
import {
  PREFERS_REDUCED_MOTION,
  convertToUnit,
  createSimpleFunctional,
  genericComponent,
  getPropertyFromItem,
  makeComponentProps,
  omit,
  pick,
  propsFactory,
  provideDefaults,
  useRender
} from "./chunk-I5IG4TPY.js";
import {
  Fragment,
  computed,
  createBaseVNode,
  createVNode,
  inject,
  mergeProps,
  nextTick,
  normalizeClass,
  normalizeStyle,
  provide,
  ref,
  shallowRef,
  toRef,
  toRefs,
  vShow,
  watch,
  withDirectives
} from "./chunk-ZS4T33C3.js";

// node_modules/vuetify/lib/components/VStepper/VStepper.js
import "/app/node_modules/vuetify/lib/components/VStepper/VStepper.css";

// node_modules/vuetify/lib/components/VStepper/shared.js
var VStepperSymbol = Symbol.for("vuetify:v-stepper");

// node_modules/vuetify/lib/components/VStepper/VStepperActions.js
var makeVStepperActionsProps = propsFactory({
  color: String,
  disabled: {
    type: [Boolean, String],
    default: false
  },
  prevText: {
    type: String,
    default: "$vuetify.stepper.prev"
  },
  nextText: {
    type: String,
    default: "$vuetify.stepper.next"
  }
}, "VStepperActions");
var VStepperActions = genericComponent()({
  name: "VStepperActions",
  props: makeVStepperActionsProps(),
  emits: {
    "click:prev": () => true,
    "click:next": () => true
  },
  setup(props, _ref) {
    let {
      emit,
      slots
    } = _ref;
    const {
      t
    } = useLocale();
    function onClickPrev() {
      emit("click:prev");
    }
    function onClickNext() {
      emit("click:next");
    }
    useRender(() => {
      const prevSlotProps = {
        onClick: onClickPrev
      };
      const nextSlotProps = {
        onClick: onClickNext
      };
      return createBaseVNode("div", {
        "class": "v-stepper-actions"
      }, [createVNode(VDefaultsProvider, {
        "defaults": {
          VBtn: {
            disabled: ["prev", true].includes(props.disabled),
            text: t(props.prevText),
            variant: "text"
          }
        }
      }, {
        default: () => {
          var _a;
          return [((_a = slots.prev) == null ? void 0 : _a.call(slots, {
            props: prevSlotProps
          })) ?? createVNode(VBtn, prevSlotProps, null)];
        }
      }), createVNode(VDefaultsProvider, {
        "defaults": {
          VBtn: {
            color: props.color,
            disabled: ["next", true].includes(props.disabled),
            text: t(props.nextText),
            variant: "tonal"
          }
        }
      }, {
        default: () => {
          var _a;
          return [((_a = slots.next) == null ? void 0 : _a.call(slots, {
            props: nextSlotProps
          })) ?? createVNode(VBtn, nextSlotProps, null)];
        }
      })]);
    });
    return {};
  }
});

// node_modules/vuetify/lib/components/VStepper/VStepperHeader.js
var VStepperHeader = createSimpleFunctional("v-stepper-header");

// node_modules/vuetify/lib/components/VStepper/VStepperItem.js
import "/app/node_modules/vuetify/lib/components/VStepper/VStepperItem.css";
var makeStepperItemProps = propsFactory({
  color: String,
  title: String,
  subtitle: String,
  complete: Boolean,
  completeIcon: {
    type: IconValue,
    default: "$complete"
  },
  editable: Boolean,
  editIcon: {
    type: IconValue,
    default: "$edit"
  },
  error: Boolean,
  errorIcon: {
    type: IconValue,
    default: "$error"
  },
  icon: IconValue,
  ripple: {
    type: [Boolean, Object],
    default: true
  },
  rules: {
    type: Array,
    default: () => []
  }
}, "StepperItem");
var makeVStepperItemProps = propsFactory({
  ...makeStepperItemProps(),
  ...makeGroupItemProps()
}, "VStepperItem");
var VStepperItem = genericComponent()({
  name: "VStepperItem",
  directives: {
    vRipple: ripple_default
  },
  props: makeVStepperItemProps(),
  emits: {
    "group:selected": (val) => true
  },
  setup(props, _ref) {
    let {
      slots
    } = _ref;
    const group = useGroupItem(props, VStepperSymbol, true);
    const step = computed(() => (group == null ? void 0 : group.value.value) ?? props.value);
    const isValid = computed(() => props.rules.every((handler) => handler() === true));
    const isClickable = computed(() => !props.disabled && props.editable);
    const canEdit = computed(() => !props.disabled && props.editable);
    const hasError = computed(() => props.error || !isValid.value);
    const hasCompleted = computed(() => props.complete || props.rules.length > 0 && isValid.value);
    const icon = computed(() => {
      if (hasError.value) return props.errorIcon;
      if (hasCompleted.value) return props.completeIcon;
      if (group.isSelected.value && props.editable) return props.editIcon;
      return props.icon;
    });
    const slotProps = computed(() => ({
      canEdit: canEdit.value,
      hasError: hasError.value,
      hasCompleted: hasCompleted.value,
      title: props.title,
      subtitle: props.subtitle,
      step: step.value,
      value: props.value
    }));
    useRender(() => {
      var _a, _b, _c;
      const hasColor = (!group || group.isSelected.value || hasCompleted.value || canEdit.value) && !hasError.value && !props.disabled;
      const hasTitle = !!(props.title != null || slots.title);
      const hasSubtitle = !!(props.subtitle != null || slots.subtitle);
      function onClick() {
        group == null ? void 0 : group.toggle();
      }
      return withDirectives(createBaseVNode("button", {
        "class": normalizeClass(["v-stepper-item", {
          "v-stepper-item--complete": hasCompleted.value,
          "v-stepper-item--disabled": props.disabled,
          "v-stepper-item--error": hasError.value
        }, group == null ? void 0 : group.selectedClass.value]),
        "disabled": !props.editable,
        "type": "button",
        "onClick": onClick
      }, [isClickable.value && genOverlays(true, "v-stepper-item"), createVNode(VAvatar, {
        "key": "stepper-avatar",
        "class": "v-stepper-item__avatar",
        "color": hasColor ? props.color : void 0,
        "size": 24
      }, {
        default: () => {
          var _a2;
          return [((_a2 = slots.icon) == null ? void 0 : _a2.call(slots, slotProps.value)) ?? (icon.value ? createVNode(VIcon, {
            "icon": icon.value
          }, null) : step.value)];
        }
      }), createBaseVNode("div", {
        "class": "v-stepper-item__content"
      }, [hasTitle && createBaseVNode("div", {
        "key": "title",
        "class": "v-stepper-item__title"
      }, [((_a = slots.title) == null ? void 0 : _a.call(slots, slotProps.value)) ?? props.title]), hasSubtitle && createBaseVNode("div", {
        "key": "subtitle",
        "class": "v-stepper-item__subtitle"
      }, [((_b = slots.subtitle) == null ? void 0 : _b.call(slots, slotProps.value)) ?? props.subtitle]), (_c = slots.default) == null ? void 0 : _c.call(slots, slotProps.value)])]), [[ripple_default, props.editable && props.ripple, null]]);
    });
    return {};
  }
});

// node_modules/vuetify/lib/components/VWindow/VWindow.js
import "/app/node_modules/vuetify/lib/components/VWindow/VWindow.css";
var VWindowSymbol = Symbol.for("vuetify:v-window");
var VWindowGroupSymbol = Symbol.for("vuetify:v-window-group");
var makeVWindowProps = propsFactory({
  continuous: Boolean,
  nextIcon: {
    type: [Boolean, String, Function, Object],
    default: "$next"
  },
  prevIcon: {
    type: [Boolean, String, Function, Object],
    default: "$prev"
  },
  reverse: Boolean,
  showArrows: {
    type: [Boolean, String],
    validator: (v) => typeof v === "boolean" || v === "hover"
  },
  verticalArrows: [Boolean, String],
  touch: {
    type: [Object, Boolean],
    default: void 0
  },
  direction: {
    type: String,
    default: "horizontal"
  },
  modelValue: null,
  disabled: Boolean,
  selectedClass: {
    type: String,
    default: "v-window-item--active"
  },
  // TODO: mandatory should probably not be exposed but do this for now
  mandatory: {
    type: [Boolean, String],
    default: "force"
  },
  crossfade: Boolean,
  transitionDuration: Number,
  ...makeComponentProps(),
  ...makeTagProps(),
  ...makeThemeProps()
}, "VWindow");
var VWindow = genericComponent()({
  name: "VWindow",
  directives: {
    vTouch: touch_default
  },
  props: makeVWindowProps(),
  emits: {
    "update:modelValue": (value) => true
  },
  setup(props, _ref) {
    let {
      slots
    } = _ref;
    const {
      themeClasses
    } = provideTheme(props);
    const {
      isRtl
    } = useRtl();
    const {
      t
    } = useLocale();
    const group = useGroup(props, VWindowGroupSymbol);
    const rootRef = ref();
    const isRtlReverse = computed(() => isRtl.value ? !props.reverse : props.reverse);
    const isReversed = shallowRef(false);
    const transition = computed(() => {
      if (props.crossfade) {
        return "v-window-crossfade-transition";
      }
      const axis = props.direction === "vertical" ? "y" : "x";
      const reverse = isRtlReverse.value ? !isReversed.value : isReversed.value;
      const direction = reverse ? "-reverse" : "";
      return `v-window-${axis}${direction}-transition`;
    });
    const transitionCount = shallowRef(0);
    const transitionHeight = ref(void 0);
    const activeIndex = computed(() => {
      return group.items.value.findIndex((item) => group.selected.value.includes(item.id));
    });
    watch(activeIndex, (newVal, oldVal) => {
      const itemsLength = group.items.value.length;
      const lastIndex = itemsLength - 1;
      if (itemsLength <= 2) {
        isReversed.value = newVal < oldVal;
      } else if (newVal === lastIndex && oldVal === 0) {
        isReversed.value = true;
      } else if (newVal === 0 && oldVal === lastIndex) {
        isReversed.value = false;
      } else {
        isReversed.value = newVal < oldVal;
      }
    });
    provide(VWindowSymbol, {
      transition,
      isReversed,
      transitionCount,
      transitionHeight,
      rootRef
    });
    const canMoveBack = toRef(() => props.continuous || activeIndex.value !== 0);
    const canMoveForward = toRef(() => props.continuous || activeIndex.value !== group.items.value.length - 1);
    function prev() {
      canMoveBack.value && group.prev();
    }
    function next() {
      canMoveForward.value && group.next();
    }
    const arrows = computed(() => {
      const arrows2 = [];
      const prevProps = {
        icon: isRtl.value ? props.nextIcon : props.prevIcon,
        class: `v-window__${isRtlReverse.value ? "right" : "left"}`,
        onClick: group.prev,
        "aria-label": t("$vuetify.carousel.prev")
      };
      arrows2.push(canMoveBack.value ? slots.prev ? slots.prev({
        props: prevProps
      }) : createVNode(VBtn, prevProps, null) : createBaseVNode("div", null, null));
      const nextProps = {
        icon: isRtl.value ? props.prevIcon : props.nextIcon,
        class: `v-window__${isRtlReverse.value ? "left" : "right"}`,
        onClick: group.next,
        "aria-label": t("$vuetify.carousel.next")
      };
      arrows2.push(canMoveForward.value ? slots.next ? slots.next({
        props: nextProps
      }) : createVNode(VBtn, nextProps, null) : createBaseVNode("div", null, null));
      return arrows2;
    });
    const touchOptions = computed(() => {
      if (props.touch === false) return props.touch;
      const options = {
        left: () => {
          isRtlReverse.value ? prev() : next();
        },
        right: () => {
          isRtlReverse.value ? next() : prev();
        },
        start: (_ref2) => {
          let {
            originalEvent
          } = _ref2;
          originalEvent.stopPropagation();
        }
      };
      return {
        ...options,
        ...props.touch === true ? {} : props.touch
      };
    });
    useRender(() => withDirectives(createVNode(props.tag, {
      "ref": rootRef,
      "class": normalizeClass(["v-window", {
        "v-window--show-arrows-on-hover": props.showArrows === "hover",
        "v-window--vertical-arrows": !!props.verticalArrows,
        "v-window--crossfade": !!props.crossfade
      }, themeClasses.value, props.class]),
      "style": normalizeStyle([props.style, props.transitionDuration && !PREFERS_REDUCED_MOTION ? {
        "--v-window-transition-duration": convertToUnit(props.transitionDuration, "ms")
      } : void 0])
    }, {
      default: () => {
        var _a, _b;
        return [createBaseVNode("div", {
          "class": "v-window__container",
          "style": {
            height: transitionHeight.value
          }
        }, [(_a = slots.default) == null ? void 0 : _a.call(slots, {
          group
        }), props.showArrows !== false && createBaseVNode("div", {
          "class": normalizeClass(["v-window__controls", {
            "v-window__controls--left": props.verticalArrows === "left" || props.verticalArrows === true
          }, {
            "v-window__controls--right": props.verticalArrows === "right"
          }])
        }, [arrows.value])]), (_b = slots.additional) == null ? void 0 : _b.call(slots, {
          group
        })];
      }
    }), [[touch_default, touchOptions.value]]));
    return {
      group
    };
  }
});

// node_modules/vuetify/lib/components/VStepper/VStepperWindow.js
var makeVStepperWindowProps = propsFactory({
  ...omit(makeVWindowProps(), ["continuous", "nextIcon", "prevIcon", "showArrows", "touch", "mandatory"])
}, "VStepperWindow");
var VStepperWindow = genericComponent()({
  name: "VStepperWindow",
  props: makeVStepperWindowProps(),
  emits: {
    "update:modelValue": (v) => true
  },
  setup(props, _ref) {
    let {
      slots
    } = _ref;
    const group = inject(VStepperSymbol, null);
    const _model = useProxiedModel(props, "modelValue");
    const model = computed({
      get() {
        var _a;
        if (_model.value != null || !group) return _model.value;
        return (_a = group.items.value.find((item) => group.selected.value.includes(item.id))) == null ? void 0 : _a.value;
      },
      set(val) {
        _model.value = val;
      }
    });
    useRender(() => {
      const windowProps = VWindow.filterProps(props);
      return createVNode(VWindow, mergeProps({
        "_as": "VStepperWindow"
      }, windowProps, {
        "modelValue": model.value,
        "onUpdate:modelValue": ($event) => model.value = $event,
        "class": ["v-stepper-window", props.class],
        "style": props.style,
        "mandatory": false,
        "touch": false
      }), slots);
    });
    return {};
  }
});

// node_modules/vuetify/lib/components/VWindow/VWindowItem.js
var makeVWindowItemProps = propsFactory({
  reverseTransition: {
    type: [Boolean, String],
    default: void 0
  },
  transition: {
    type: [Boolean, String],
    default: void 0
  },
  ...makeComponentProps(),
  ...makeGroupItemProps(),
  ...makeLazyProps()
}, "VWindowItem");
var VWindowItem = genericComponent()({
  name: "VWindowItem",
  directives: {
    vTouch: touch_default
  },
  props: makeVWindowItemProps(),
  emits: {
    "group:selected": (val) => true
  },
  setup(props, _ref) {
    let {
      slots
    } = _ref;
    const window = inject(VWindowSymbol);
    const groupItem = useGroupItem(props, VWindowGroupSymbol);
    const {
      isBooted
    } = useSsrBoot();
    if (!window || !groupItem) throw new Error("[Vuetify] VWindowItem must be used inside VWindow");
    const isTransitioning = shallowRef(false);
    const hasTransition = computed(() => isBooted.value && (window.isReversed.value ? props.reverseTransition !== false : props.transition !== false));
    function onAfterTransition() {
      if (!isTransitioning.value || !window) {
        return;
      }
      isTransitioning.value = false;
      if (window.transitionCount.value > 0) {
        window.transitionCount.value -= 1;
        if (window.transitionCount.value === 0) {
          window.transitionHeight.value = void 0;
        }
      }
    }
    function onBeforeTransition() {
      var _a;
      if (isTransitioning.value || !window) {
        return;
      }
      isTransitioning.value = true;
      if (window.transitionCount.value === 0) {
        window.transitionHeight.value = convertToUnit((_a = window.rootRef.value) == null ? void 0 : _a.clientHeight);
      }
      window.transitionCount.value += 1;
    }
    function onTransitionCancelled() {
      onAfterTransition();
    }
    function onEnterTransition(el) {
      if (!isTransitioning.value) {
        return;
      }
      nextTick(() => {
        if (!hasTransition.value || !isTransitioning.value || !window) {
          return;
        }
        window.transitionHeight.value = convertToUnit(el.clientHeight);
      });
    }
    const transition = computed(() => {
      const name = window.isReversed.value ? props.reverseTransition : props.transition;
      return !hasTransition.value ? false : {
        name: typeof name !== "string" ? window.transition.value : name,
        onBeforeEnter: onBeforeTransition,
        onAfterEnter: onAfterTransition,
        onEnterCancelled: onTransitionCancelled,
        onBeforeLeave: onBeforeTransition,
        onAfterLeave: onAfterTransition,
        onLeaveCancelled: onTransitionCancelled,
        onEnter: onEnterTransition
      };
    });
    const {
      hasContent
    } = useLazy(props, groupItem.isSelected);
    useRender(() => createVNode(MaybeTransition, {
      "transition": transition.value,
      "disabled": !isBooted.value
    }, {
      default: () => {
        var _a;
        return [withDirectives(createBaseVNode("div", {
          "class": normalizeClass(["v-window-item", groupItem.selectedClass.value, props.class]),
          "style": normalizeStyle(props.style)
        }, [hasContent.value && ((_a = slots.default) == null ? void 0 : _a.call(slots))]), [[vShow, groupItem.isSelected.value]])];
      }
    }));
    return {
      groupItem
    };
  }
});

// node_modules/vuetify/lib/components/VStepper/VStepperWindowItem.js
var makeVStepperWindowItemProps = propsFactory({
  ...makeVWindowItemProps()
}, "VStepperWindowItem");
var VStepperWindowItem = genericComponent()({
  name: "VStepperWindowItem",
  props: makeVStepperWindowItemProps(),
  setup(props, _ref) {
    let {
      slots
    } = _ref;
    useRender(() => {
      const windowItemProps = VWindowItem.filterProps(props);
      return createVNode(VWindowItem, mergeProps({
        "_as": "VStepperWindowItem"
      }, windowItemProps, {
        "class": ["v-stepper-window-item", props.class],
        "style": props.style
      }), slots);
    });
    return {};
  }
});

// node_modules/vuetify/lib/components/VStepper/VStepper.js
var makeStepperProps = propsFactory({
  altLabels: Boolean,
  bgColor: String,
  completeIcon: IconValue,
  editIcon: IconValue,
  editable: Boolean,
  errorIcon: IconValue,
  hideActions: Boolean,
  items: {
    type: Array,
    default: () => []
  },
  itemTitle: {
    type: String,
    default: "title"
  },
  itemValue: {
    type: String,
    default: "value"
  },
  nonLinear: Boolean,
  flat: Boolean,
  ...makeDisplayProps()
}, "Stepper");
var makeVStepperProps = propsFactory({
  ...makeStepperProps(),
  ...makeGroupProps({
    mandatory: "force",
    selectedClass: "v-stepper-item--selected"
  }),
  ...makeVSheetProps(),
  ...pick(makeVStepperActionsProps(), ["prevText", "nextText"])
}, "VStepper");
var VStepper = genericComponent()({
  name: "VStepper",
  props: makeVStepperProps(),
  emits: {
    "update:modelValue": (v) => true
  },
  setup(props, _ref) {
    let {
      slots
    } = _ref;
    const {
      items: _items,
      next,
      prev,
      selected
    } = useGroup(props, VStepperSymbol);
    const {
      displayClasses,
      mobile
    } = useDisplay(props);
    const {
      completeIcon,
      editIcon,
      errorIcon,
      color,
      editable,
      prevText,
      nextText
    } = toRefs(props);
    const items = computed(() => props.items.map((item, index) => {
      const title = getPropertyFromItem(item, props.itemTitle, item);
      const value = getPropertyFromItem(item, props.itemValue, index + 1);
      return {
        title,
        value,
        raw: item
      };
    }));
    const activeIndex = computed(() => {
      return _items.value.findIndex((item) => selected.value.includes(item.id));
    });
    const disabled = computed(() => {
      if (props.disabled) return props.disabled;
      if (activeIndex.value === 0) return "prev";
      if (activeIndex.value === _items.value.length - 1) return "next";
      return false;
    });
    provideDefaults({
      VStepperItem: {
        editable,
        errorIcon,
        completeIcon,
        editIcon,
        prevText,
        nextText
      },
      VStepperActions: {
        color,
        disabled,
        prevText,
        nextText
      }
    });
    useRender(() => {
      const sheetProps = VSheet.filterProps(props);
      const hasHeader = !!(slots.header || props.items.length);
      const hasWindow = props.items.length > 0;
      const hasActions = !props.hideActions && !!(hasWindow || slots.actions);
      return createVNode(VSheet, mergeProps(sheetProps, {
        "color": props.bgColor,
        "class": ["v-stepper", {
          "v-stepper--alt-labels": props.altLabels,
          "v-stepper--flat": props.flat,
          "v-stepper--non-linear": props.nonLinear,
          "v-stepper--mobile": mobile.value
        }, displayClasses.value, props.class],
        "style": props.style
      }), {
        default: () => {
          var _a, _b;
          return [hasHeader && createVNode(VStepperHeader, {
            "key": "stepper-header"
          }, {
            default: () => [items.value.map((_ref2, index) => {
              let {
                raw,
                ...item
              } = _ref2;
              return createBaseVNode(Fragment, null, [!!index && createVNode(VDivider, null, null), createVNode(VStepperItem, item, {
                default: slots[`header-item.${item.value}`] ?? slots.header,
                icon: slots.icon,
                title: slots.title,
                subtitle: slots.subtitle
              })]);
            })]
          }), hasWindow && createVNode(VStepperWindow, {
            "key": "stepper-window"
          }, {
            default: () => [items.value.map((item) => createVNode(VStepperWindowItem, {
              "value": item.value
            }, {
              default: () => {
                var _a2, _b2;
                return ((_a2 = slots[`item.${item.value}`]) == null ? void 0 : _a2.call(slots, item)) ?? ((_b2 = slots.item) == null ? void 0 : _b2.call(slots, item));
              }
            }))]
          }), (_a = slots.default) == null ? void 0 : _a.call(slots, {
            prev,
            next
          }), hasActions && (((_b = slots.actions) == null ? void 0 : _b.call(slots, {
            next,
            prev
          })) ?? createVNode(VStepperActions, {
            "key": "stepper-actions",
            "onClick:prev": prev,
            "onClick:next": next
          }, slots))];
        }
      });
    });
    return {
      prev,
      next
    };
  }
});

export {
  VStepperActions,
  VStepperHeader,
  VStepperItem,
  makeVWindowProps,
  VWindow,
  VStepperWindow,
  makeVWindowItemProps,
  VWindowItem,
  VStepperWindowItem,
  VStepper
};
//# sourceMappingURL=chunk-7VGRFFKP.js.map
